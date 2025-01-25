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

    int shmid = createSharedMemory();
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
            poolId = RECRE;  // teen needs to be in recre pool
        }
    }

    int msgid = create_message_queue();
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
    msg.mtype = randRange(1, 2);  // random vip right now
    msg.pid = getpid();           // PID adult
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

    if (response.status != 1) {
        printf(YELLOW "Client (%d): Access denied. Reason: %s" END "\n",
               getpid(), response.text);
        return 0;
    } else {
        if (msg.mtype == 2) {
            printf(GREEN "VIP Client (%d): Access granted. Info: %s" END "\n",
                   msg.pid, response.text);
        } else {
            printf(GREEN "Client (%d): Access granted. Info: %s" END "\n",
                   msg.pid, response.text);
        }

        //
        const char* fifoName = NULL;
        switch (msg.poolId) {
            case OLIMPIC:
                fifoName = "./fifo_olimpic";
                break;
            case RECRE:
                fifoName = "./fifo_recre";
                break;
            case CHILD:
                fifoName = "./fifo_child";
                break;
            default:
                fprintf(stderr, RED "Unknown pool ID" END "\n");
                exit(EXIT_FAILURE);
                break;
        }
        if (fifoName) {
            addPidToFifo(fifoName, getpid());
        }
        if (hasChild) {
            // create only if you can enter
            if (pthread_create(&thread, NULL, childThread, &child) != 0) {
                perror("Client: pthread_create");
                exit(EXIT_FAILURE);
            }
        }
        // Zakończenie wątku dziecka (jeśli istnieje)
        if (hasChild) {
            pthread_join(thread, NULL);
            printf("Client PID=%d has left the pool with child.\n", getpid());
        } else {
            printf("Client PID=%d has left the pool.\n", getpid());
        }
        // sleep(1);                            // entered for 1sec
        pthread_mutex_lock(&shdata->mutex);  // locked
        printf("locked.\n");

        switch (msg.poolId) {
            case OLIMPIC:
                shdata->olimpicCount--;
                // if adult in olimpic have child, child will be in recre
                if (msg.hasChild) {
                    shdata->recreCount--;
                    shdata->recreSumAge -= msg.childAge;
                }
                break;
            case RECRE:
                shdata->recreCount--;
                shdata->recreSumAge -= msg.adultAge;
                // if adult in recre have child, child will be in recre
                if (msg.hasChild) {
                    shdata->recreCount--;
                    shdata->recreSumAge -= msg.childAge;
                }

                break;
            case CHILD:
                shdata->childCount -= 2;  // -2 because guardian will also leave
                break;
        }
        pthread_mutex_unlock(&shdata->mutex);  // unlock
        if (hasChild) {
            printf(BLUE "Im %d leaving the pool with child \n" END "\n",
                   msg.pid);

        } else {
            printf(BLUE "Im %d leaving the pool \n" END "\n", msg.pid);
        }
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, RED "Client: problem with detach shmem." END "\n");
    }
    return 0;
}