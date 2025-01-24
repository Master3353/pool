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
        fprintf(stderr, RED "Thread: error getSharedMemory." END "\n");
        pthread_exit(NULL);
    }
    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, RED "Thread: error attachSharedMemory." END "\n");
        pthread_exit(NULL);
    }
    // test
    // sleep(5);

    detachSharedMemory(shdata);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    srand(time(NULL) ^ getpid());
    if (argc < 1) {
        fprintf(stderr, RED "Wrong arguments: %s" END "\n", argv[0]);
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
        fprintf(stderr, RED "Client: Probelm with getting shmem." END "\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, RED "Client: problem with attach shmem." END "\n");
        exit(EXIT_FAILURE);
    }
    if (response.status != 1) {
        printf(YELLOW "Client (%d): Access denied. Reason: %s" END "\n",
               getpid(), response.text);
        return 0;
    } else {
        printf(GREEN "Client (%d): Access granted. Info: %s" END "\n", msg.pid,
               response.text);
        if (hasChild) {
            // create only if you can enter
            if (pthread_create(&thread, NULL, childThread, &child) != 0) {
                perror("Client: pthread_create");
                exit(EXIT_FAILURE);
            }
        }
        // when one is done using - empty space
        if (hasChild) {
            pthread_join(thread, NULL);
            sleep(10);
            // printf(BLUE "Child of %d ending." END "\n", msg.pid);
        }
        sleep(5);  // entered for 5sec
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, RED "Client: problem with detach shmem." END "\n");
    }
    if (hasChild) {
        printf(BLUE "Im %d leaving the pool with child \n" END "\n", msg.pid);

    } else {
        printf(BLUE "Im %d leaving the pool \n" END "\n", msg.pid);
    }
    return 0;
}