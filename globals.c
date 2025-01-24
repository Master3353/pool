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

/*
    *Creating new FIFO for communication between lifeguards and cashier
    *deleting any old file, when there is one

*/
int createFifo() {
    unlink(LIFEGUARD_FIFO);  // delete old FIFO
    if (mkfifo(LIFEGUARD_FIFO, 0600) == -1) {
        perror("createFifo: mkfifo");
        return -1;
    }
    return 0;
}

int openFifoRead() {
    int fd = open(LIFEGUARD_FIFO, O_RDONLY);
    if (fd < 0) {
        perror("openFifoRead");
    }
    return fd;
}

int openFifoWrite() {
    int fd = open(LIFEGUARD_FIFO, O_WRONLY);
    if (fd < 0) {
        perror("openFifoWrite");
    }
    return fd;
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