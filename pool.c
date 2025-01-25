#include "globals.h"

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
    // create FIFOs for communication between lifeguards and clients
    createFifo("fifo_olimpic");
    createFifo("fifo_recre");
    createFifo("fifo_child");
    dummyRead("fifo_olimpic");
    dummyRead("fifo_recre");
    dummyRead("fifo_child");
    pid_t cashierPid = fork();
    char cashierPidStr[20];
    snprintf(cashierPidStr, sizeof(cashierPidStr), "%d", cashierPid);

    if (cashierPid == 0) {
        execl("./cashier", "./cashier", NULL);
        perror("Error with execl cashier");
        exit(EXIT_FAILURE);
    } else if (cashierPid < 0) {
        perror("Error with forking cashier");
        exit(EXIT_FAILURE);
    }
    // pid_t childLifeguardPid = fork();
    // if (childLifeguardPid == 0) {
    //     execl("./lifeguard", "./lifeguard", "1", cashierPidStr, NULL);
    //     perror("Error with execl a lifeguard 1");
    //     exit(EXIT_FAILURE);
    // } else if (childLifeguardPid < 0) {
    //     perror("Error with forking lifeguard 1");
    //     exit(EXIT_FAILURE);
    // }
    // pid_t recreLifeguardPid = fork();
    // if (recreLifeguardPid == 0) {
    //     execl("./lifeguard", "./lifeguard", "2", cashierPidStr, NULL);
    //     perror("Error with execl a lifeguard 2");
    //     exit(EXIT_FAILURE);
    // } else if (recreLifeguardPid < 0) {
    //     perror("Error with forking lifeguard 2");
    //     exit(EXIT_FAILURE);
    // }
    // pid_t olimpicLifeguardPid = fork();
    // if (olimpicLifeguardPid == 0) {
    //     execl("./lifeguard", "./lifeguard", "3", cashierPidStr, NULL);
    //     perror("Error with execl a lifeguard 3");
    //     exit(EXIT_FAILURE);
    // } else if (olimpicLifeguardPid < 0) {
    //     perror("Error with forking lifeguard 3");
    //     exit(EXIT_FAILURE);
    // }
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
        usleep(1000000);
    }
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
    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Pool: problem with detach shmem.\n");
    }
    if (destroySharedMemory(shmid) == -1) {
        fprintf(stderr, "Pool: problem with deleting shmem.\n");
    }

    return 0;
}