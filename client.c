#include "globals.h"

typedef struct {
    int childAge;
    int hasPampers;
    int adultAge;
    int targetPool;
} ChildGuardian;
// random age generator
int randRange(int min, int max) { return min + rand() % (max - min + 1); }
void* childGuardianThread(void* arg) {
    ChildGuardian* data = (ChildGuardian*)arg;

    int shmid = getSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Thread: getSharedMemory error.\n");
        pthread_exit(NULL);
    }
    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Thread: attachSharedMemory error.\n");
        pthread_exit(NULL);
    }

    printf("[Thread] ending .\n");

    detachSharedMemory(shdata);

    pthread_exit(NULL);
}
int main(int argc, char* argv[]) {
    srand(time(NULL) ^ getpid());
    if (argc < 1) {
        fprintf(stderr, "Wrong arguments: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int age = randRange(1, 70);
    int poolID;
    int pampers = 0;
    if (age < 6) {
        poolID = CHILD;
        if (age < 4) {
            pampers = 1;
        }
    } else if (age < 18 && age > 5) {
        poolID = RECRE;
    } else {
        poolID = randRange(1, 2);
    }

    printf("Client: Random age: %d years, Pool: %d\n", age, poolID);

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
    printf("Hello from client aged %d on pool %d \n", age, poolID);
}