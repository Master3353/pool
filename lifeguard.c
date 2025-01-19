#include "globals.h"
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Użycie: %s <poolId>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int poolId = atoi(argv[1]);

    printf("Hello from lifeguard on pool %d \n", poolId);

    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Lifeguard: Probelm with getting shmem.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Lifeguard: problem with attach shmem.\n");
        exit(EXIT_FAILURE);
    }

    printf("Lifeguard: i have shmem! olimpicCount: %d\n", shdata->olimpicCount);

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Lifeguard: problem with detach shmem.\n");
    }

    return 0;
}