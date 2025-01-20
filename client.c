#include "globals.h"

typedef struct {
    int childAge;
    int hasPampers;
    int targetPool;
} Child;
// random age generator
int randRange(int min, int max) { return min + rand() % (max - min + 1); }
void* childThread(void* arg) {
    Child* child = (Child*)arg;

    int shmid = getSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Thread: error getSharedMemory.\n");
        pthread_exit(NULL);
    }
    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Thread: error attachSharedMemory.\n");
        pthread_exit(NULL);
    }
    // test
    sleep(5);

    printf("[Thread] Child ending.\n");

    detachSharedMemory(shdata);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    srand(time(NULL) ^ getpid());
    if (argc < 1) {
        fprintf(stderr, "Wrong arguments: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int hasChild = randRange(0, 1);  // 0 - no child, 1 - child

    Child child;
    pthread_t thread;
    if (hasChild) {
        child.childAge = randRange(1, 9);
        child.hasPampers = (child.childAge <= 3) ? 1 : 0;
        child.targetPool =
            (child.childAge <= 5)
                ? CHILD
                : RECRE;  // 5 and younger to CHILD, older to recre

        printf("Client: I have a child %d years old. Pampers: %s\n",
               child.childAge, child.hasPampers ? "yes" : "no");

        if (pthread_create(&thread, NULL, childThread, &child) != 0) {
            perror("Client: pthread_create");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Client without Child.\n");
    }

    int poolId;
    int age;
    if (hasChild && child.childAge <= 3) {
        age = randRange(18, 70);
        poolId = 3;  // need to be with child when is young
    } else {
        age = randRange(10, 70);
        poolId = randRange(1, 2);
    }

    printf("Client: Random age: %d years, Pool: %d\n", age, poolId);

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
    printf("Im %d entering the pool \n", getpid());
    // temporary for check
    sleep(5);
    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Client: problem with detach shmem.\n");
    }
    if (hasChild) {
        pthread_join(thread, NULL);
        printf("Client: Child ended.\n");
    }
    printf("Im %d leaving the pool \n", getpid());
    return 0;
}