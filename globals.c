#include "globals.h"
int createSharedMemory() {
    int shmid =
        shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | IPC_EXCL | 0600);
    if (shmid == -1) {
        if (errno == EEXIST) {
            shmid = shmget(SHM_KEY, sizeof(SharedMemory), 0600);
            if (shmid == -1) {
                perror("shmget exsist (createSharedMemory)");
                return -1;
            }
        } else {
            perror("shmget creating (createSharedMemory)");
            return -1;
        }
    } else {
        SharedMemory* shdata = attachSharedMemory(shmid);
        if (shdata == (void*)-1) {
            // cant attach, destroy
            destroySharedMemory(shmid);
            return -1;
        }
        initializeSharedData(shdata);
        if (detachSharedMemory(shdata) == -1) {
            perror(
                "detachSharedMemory after initialize (initializeSharedData)");
            // countinue besides problem with detach
        }
    }
    return shmid;
}

int getSharedMemory() {
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), 0600);
    if (shmid == -1) {
        perror("problem with shmget (getSharedMemory)");
        return -1;
    }
    return shmid;
}

SharedMemory* attachSharedMemory(int shmid) {
    SharedMemory* shdata = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shdata == (void*)-1) {
        perror("problem with shmat (attach)");
    }
    return shdata;
}

int detachSharedMemory(SharedMemory* shdata) {
    if (shmdt(shdata) == -1) {
        perror("problem with shmdt (detachSharedMemory)");
        return -1;
    }
    return 0;
}

int destroySharedMemory(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("problem with shmctl (destroySharedMemory)");
        return -1;
    }
    return 0;
}

void initializeSharedData(SharedMemory* shdata) {
    if (shdata == NULL) {
        fprintf(stderr, "initializeSharedData: shdata is NULL\n");
        return;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    if (pthread_mutex_init(&shdata->mutex, &attr) != 0) {
        perror("Cashier: Mutex initialization failed");
        exit(EXIT_FAILURE);
    }

    shdata->olimpicCount = 0;
    shdata->recreCount = 0;
    shdata->childCount = 0;
    shdata->recreSumAge = 0;
    shdata->isOlimpicOpen = 1;
    shdata->isRecreOpen = 1;
    shdata->isChildOpen = 1;
    shdata->isFacilityClosed = 0;
}
int randRange(int min, int max) { return min + rand() % (max - min + 1); }

/*
 *FIFO for keeping track of clients in specific pools
 *Will be needed for closing pools, so lifeguard can force clients to leave
 */

void createFifo(const char* fifoName) {
    if (mkfifo(fifoName, 0600) == -1 && errno != EEXIST) {
        perror("Error creating FIFO");
        exit(EXIT_FAILURE);
    } else {
        printf("Created FIFO %s\n", fifoName);
    }
}
int initFifoSemaphore() {
    int semid = semget(FIFO_SEM_KEY, 1, IPC_CREAT | 0600);  // 1 semafor
    if (semid == -1) {
        perror("Error creating semaphore");
        exit(EXIT_FAILURE);
    }

    // Ustawienie wartości początkowej semafora na 1 (dostępne)
    if (semctl(semid, 0, SETVAL, 1) == -1) {
        perror("Error initializing semaphore");
        exit(EXIT_FAILURE);
    }

    return semid;
}
int initSemaphore() {
    int semid = semget(SEM_KEY, SEM_COUNT, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("Cashier: semget error");
        exit(EXIT_FAILURE);
    }

    union semun {
        int val;
        struct semid_ds* buf;
        unsigned short* array;
    } arg;

    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("Cashier: semctl error");
        exit(EXIT_FAILURE);
    }

    return semid;
}
void fifoSemaphoreLock(int semid) {
    struct sembuf sb = {0, -1, 0};  // Opis operacji P (zmniejszenie o 1)
    while (semop(semid, &sb, 1) == -1) {
        if (errno == EINTR) {
            continue;
        } else {
            perror("Error locking semaphore");
            exit(EXIT_FAILURE);
        }
    }
}

// Funkcja odblokowująca semafor
void fifoSemaphoreUnlock(int semid) {
    struct sembuf sb = {0, 1, 0};  // Opis operacji V (zwiększenie o 1)
    while (semop(semid, &sb, 1) == -1) {
        if (errno == EINTR) {
            continue;
        } else {
            perror("Error unlocking semaphore");
            exit(EXIT_FAILURE);
        }
    }
}

void addPidToFifo(const char* fifoName, pid_t pid, int fifoSemid) {
    fifoSemaphoreLock(fifoSemid);
    int fd = open(fifoName, O_WRONLY | O_NONBLOCK);
    if (access(fifoName, F_OK) != 0) {
        fprintf(stderr, "FIFO %s does not exist\n", fifoName);
        return;
    }
    if (fd == -1) {
        printf("FIFO %s\n", fifoName);
        perror("Error opening FIFO");
        return;
    }

    if (dprintf(fd, "%d\n", pid) < 0) {
        perror("Error writing PID to FIFO");
    }

    close(fd);

    fifoSemaphoreUnlock(fifoSemid);
}

void checkInput() {
    if (MAX_CAPACITY_CHILD < 2) {
        fprintf(stderr, "Child pool needs to have at least 2 spots.\n");
        exit(1);
    }
    if (MAX_CAPACITY_OLIMPIC < 1) {
        fprintf(stderr, "Olimpic pool needs to have at least 1 spot.\n");
        exit(2);
    }
    if (MAX_CAPACITY_RECRE < 1) {
        fprintf(stderr, "Recreational pool needs to have at least 1 spot.\n");
        exit(3);
    }
}