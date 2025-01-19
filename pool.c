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

    pid_t cashierPid = fork();
    if (cashierPid == 0) {
        execl("./cashier", "./cashier", NULL);
        perror("Error with execl cashier");
        exit(EXIT_FAILURE);
    } else if (cashierPid < 0) {
        perror("Error with forking cashier");
        exit(EXIT_FAILURE);
    }
    // for testing purposes 1 lifeguard
    pid_t lifeguardPid = fork();
    if (lifeguardPid == 0) {
        execl("./lifeguard", "./lifeguard", "1", NULL);
        perror("Error with execl a lifeguard");
        exit(EXIT_FAILURE);
    } else if (lifeguardPid < 0) {
        perror("Error with forking lifeguard");
        exit(EXIT_FAILURE);
    }
    // Making 5 clients
    for (int i = 0; i < 5; i++) {
        pid_t clientPid = fork();
        if (clientPid == 0) {
            char ageStr[10];
            char poolStr[10];
            sprintf(ageStr, "%d", 20 + i * 2);  // increasing age
            sprintf(poolStr, "%d", 2);          // 2 = recre pool
            execl("./client", "./client", ageStr, poolStr, NULL);
            perror("Error with execl client");
            exit(EXIT_FAILURE);
        } else if (clientPid < 0) {
            perror("Error with forking client");
        }
    }

    wait(NULL);
    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Pool: problem with detach shmem.\n");
    }
    if (destroySharedMemory(shmid) == -1) {
        fprintf(stderr, "Pool: problem with deleting shmem.\n");
    }

    return 0;
}