#include "globals.h"
#include "msg_struct.h"

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

    } else {
        printf("Client without Child.\n");
    }

    int poolId;
    int adultAge;
    if (hasChild && child.childAge <= 3) {
        adultAge = randRange(18, 70);
        poolId = 3;  // need to be with child when is young
    } else {
        adultAge = randRange(10, 70);
        if (adultAge >= 18) {
            poolId = randRange(1, 2);
        } else {
            poolId = 2;
        }
    }

    printf("Client: Random age: %d years, Pool: %d\n", adultAge, poolId);

    int msgid = msgget(MSG_QUEUE_KEY, 0600);
    if (msgid == -1) {
        perror("Client: msgget");
        exit(EXIT_FAILURE);
    }
    msg_t msg;
    msg.mtype = 1;       // normal
    msg.pid = getpid();  // PID adult
    msg.adultAge = adultAge;
    msg.poolId = poolId;
    msg.hasChild = hasChild;
    msg.childAge = hasChild ? child.childAge : 0;
    msg.hasPampers = hasChild ? child.hasPampers : 0;
    msg.status = 0;  // cashier will input this
    strcpy(msg.text, "Request to enter the pool");

    if (msgsnd(msgid, &msg, sizeof(msg_t) - sizeof(long), 0) == -1) {
        perror("Client: msgsnd request");
        exit(EXIT_FAILURE);
    }
    printf("Client (parent): Sent request to cashier.\n");

    msg_t response;
    if (msgrcv(msgid, &response, sizeof(msg_t) - sizeof(long), getpid(), 0) ==
        -1) {
        perror("Client: msgrcv response");
        exit(EXIT_FAILURE);
    }

    if (response.status != 0) {
        printf("Client (parent): Access denied. Reason: %s\n", response.text);
        return 0;
    } else {
        printf("Client (parent): Access granted. Info: %s\n", response.text);
    }
    if (hasChild) {
        // create only if you can enter
        if (pthread_create(&thread, NULL, childThread, &child) != 0) {
            perror("Client: pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    printf("Client (parent): I'm using the pool now...\n");
    sleep(5);

    printf("Client (parent): I'm done using the pool.\n");

    // shmem
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