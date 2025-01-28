#include "globals.h"
pid_t lifeguardPids[3];

void sendSignalToLifeguards(int signal) {
    for (int i = 0; i < 4; i++) {
        if (lifeguardPids[i] > 0) {
            kill(lifeguardPids[i], signal);  // Wyślij sygnał do ratownika
        }
    }
}

int main() {
    checkInput();
    printf("Just checking \n");

    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Pool: Probelm with getting shmem.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Pool: problem with attach shmem.\n");
        exit(EXIT_FAILURE);
    }
    int fifoSemid = initFifoSemaphore();
    int semid = initSemaphore();

    // create FIFOs for communication between lifeguards and clients
    createFifo("fifo_olimpic");
    createFifo("fifo_recre");
    createFifo("fifo_child");

    char oppeningTime[20];
    snprintf(oppeningTime, sizeof(oppeningTime), "%s", "15");
    pid_t cashierPid = fork();

    if (cashierPid == 0) {
        execl("./cashier", "./cashier", oppeningTime, NULL);
        perror("Error with execl cashier");
        exit(EXIT_FAILURE);
    } else if (cashierPid < 0) {
        perror("Error with forking cashier");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 3; i++) {
        lifeguardPids[i] = fork();
        if (lifeguardPids[i] == 0) {
            char poolIdStr[10];
            snprintf(poolIdStr, sizeof(poolIdStr), "%d", i);
            execl("./lifeguard", "./lifeguard", poolIdStr, oppeningTime, NULL);
            perror("Error with execl lifeguard");
            exit(EXIT_FAILURE);
        } else if (lifeguardPids[i] < 0) {
            perror("Error with forking lifeguard");
            exit(EXIT_FAILURE);
        }
    }
    // Making clients
    for (int i = 0; i < 10; i++) {
        pid_t clientPid = fork();
        if (clientPid == 0) {
            execl("./client", "./client", NULL);
            perror("Error with execl client");
            exit(EXIT_FAILURE);
        } else if (clientPid < 0) {
            perror("Error with forking client");
        }
        // sleep(1);
    }

    int randomTime = randRange(0, 2);
    sleep(randomTime);  // Czekaj na losowy moment
    printf(RED "Pool: Closing facility for water change." END "\n");

    pthread_mutex_lock(&shdata->mutex);
    shdata->isFacilityClosed = 1;  // Ustaw zamknięcie obiektu
    pthread_mutex_unlock(&shdata->mutex);

    sendSignalToLifeguards(SIGUSR1);  // Wyślij sygnał do wszystkich

    // simulation water change
    sleep(5);
    printf(RED "Pool: Reopening facility after water change." END "\n");

    pthread_mutex_lock(&shdata->mutex);
    shdata->isFacilityClosed = 0;  // Otwórz obiekt po wymianie wody
    pthread_mutex_unlock(&shdata->mutex);

    sendSignalToLifeguards(
        SIGUSR2);  // Wyślij sygnał otwarcia do wszystkich ratowników

    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            // printf("Child with PID %d exited with status %d.\n", wpid,
            //        WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            // printf("Child with PID %d was killed by signal %d.\n", wpid,
            //       WTERMSIG(status));
        } else {
            printf("Child with PID %d terminated abnormally.\n", wpid);
        }
    }

    printf("All children have ended. Ending program\n");
    if (semctl(fifoSemid, 0, IPC_RMID) == -1) {
        perror("Error removing semaphore");
    }
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("Cashier: semctl IPC_RMID");
    }
    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Pool: problem with detach shmem.\n");
    }
    if (destroySharedMemory(shmid) == -1) {
        fprintf(stderr, "Pool: problem with deleting shmem.\n");
    }

    return 0;
}