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
    // sleep(5);

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
    msg_t msg;
    Child child;
    pthread_t thread;
    if (hasChild) {
        child.childAge = randRange(1, 9);
        child.hasPampers = (child.childAge < 3) ? 1 : 0;
        child.targetPool =
            (child.childAge < 5)
                ? CHILD
                : RECRE;  // 5 and younger to CHILD, older to recre
    }
    int poolId;
    int adultAge;
    if (hasChild) {
        adultAge = randRange(18, 70);
        msg.childAge = child.childAge;
        if (child.childAge < 5) {
            poolId = CHILD;  // needs to be with child
            msg.childPoolId = CHILD;
        } else {
            poolId =
                randRange(OLIMPIC, RECRE);  // adult can go to olimpic/recre
        }
    } else {
        adultAge = randRange(10, 70);
        msg.childPoolId = RECRE;
        if (adultAge >= 18) {
            poolId =
                randRange(OLIMPIC, RECRE);  // adult can go to olimpic/recre
        } else {
            poolId = RECRE;  // tenn needs to be in recre pool
        }
    }

    int msgid = create_message_queue();

    msg.mtype = 1;       // normal
    msg.pid = getpid();  // PID adult
    msg.adultAge = adultAge;
    msg.poolId = poolId;
    msg.isVip = 0;
    msg.hasChild = hasChild;
    msg.childAge = 0;
    if (hasChild) {
        msg.childAge = child.childAge;
        // pool for kid
        msg.childPoolId = child.targetPool;
        msg.hasPampers = child.hasPampers;
    } else {
        msg.childAge = 0;
        msg.childPoolId = -1;  // cashier knows, that its solo adult
        msg.hasPampers = 0;
    }
    msg.status = 0;  // cashier will input this
    strcpy(msg.text, "Request to enter the pool");
    // sleep(1);
    if (msgsnd(msgid, &msg, sizeof(msg_t) - sizeof(long), 0) == -1) {
        perror("Client: msgsnd request");
        exit(EXIT_FAILURE);
    }
    // printf("Client (parent): Sent request to cashier.\n");

    msg_t response;
    if (msgrcv(msgid, &response, sizeof(msg_t) - sizeof(long), getpid(), 0) ==
        -1) {
        perror("Client: msgrcv response");
        exit(EXIT_FAILURE);
    } else {
        // printf("received\n");
    }
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
    if (response.status != 1) {
        printf("Client (parent): Access denied. Reason: %s\n", response.text);
        return 0;
    } else {
        printf("Client (parent): Access granted. Info: %s\n", response.text);
        if (hasChild) {
            // create only if you can enter
            if (pthread_create(&thread, NULL, childThread, &child) != 0) {
                perror("Client: pthread_create");
                exit(EXIT_FAILURE);
            }
        }
        printf("Client: I'm done using the pool.\n");
        // when one is done using - empty space
        if (hasChild) {
            pthread_join(thread, NULL);
            printf("Client: Child ended.\n");
        }
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Client: problem with detach shmem.\n");
    }

    printf("Im %d leaving the pool \n", getpid());
    return 0;
}