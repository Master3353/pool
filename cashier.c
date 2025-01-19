#include "globals.h"
int main(void) {
    printf("Hello from cashier!\n");

    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Cashier: Probelm with getting shmem.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Cashier: problem with attach shmem.\n");
        exit(EXIT_FAILURE);
    }

    printf("Cashier: i have shmem! olimpicCount: %d\n", shdata->olimpicCount);
    shdata->olimpicCount += 1;

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Cashier: problem with detach shmem.\n");
    }
    return 0;
}