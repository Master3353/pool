#include "globals.h"
int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Użycie: %s <age> <poolId>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int age = atoi(argv[1]);
    int poolId = atoi(argv[2]);

    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Client: Probelm with getting shmem.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Client: problem with attach shmem.\n");
        exit(EXIT_FAILURE);
    }

    printf("Client: i have shmem! olimpicCount: %d\n", shdata->olimpicCount);
    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Client: problem with detach shmem.\n");
    }
    printf("Hello from client aged %d on pool %d \n", age, poolId);
}