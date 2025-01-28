

#include <signal.h>

#include "globals.h"
#include "msg_struct.h"
volatile sig_atomic_t time_up = 0;
static int closedPoolsCount = 0;  // track closed pools

void handleAlarm(int sig) { time_up = 1; }

void validateCounters(SharedMemory *shdata) {
    if (shdata->olimpicCount < 0) shdata->olimpicCount = 0;
    if (shdata->recreCount < 0) shdata->recreCount = 0;
    if (shdata->childCount < 0) shdata->childCount = 0;
}
/*
    * Function for receiving messages from queue
    * prioritize clients with VIP status
    * mtype = 1 means VIP, mtype = 2 means normal client

*/
int receiveMessage(int msgid, msg_t *msg) {
    if (msgrcv(msgid, msg, sizeof(msg_t) - sizeof(long), 1, IPC_NOWAIT) != -1) {
        return 1;  // VIP
    }
    if (msgrcv(msgid, msg, sizeof(msg_t) - sizeof(long), 2, IPC_NOWAIT) != -1) {
        return 2;  // normal
    }
    return 0;
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <oppeningTime>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int oppeningTime = atoi(argv[1]);
    int msgid = create_message_queue();
    int semid = initSemaphore();
    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, RED "Cashier: Probelm with getting shmem." END "\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory *shdata = attachSharedMemory(shmid);
    if (shdata == (void *)-1) {
        fprintf(stderr, RED "Cashier: problem with attach shmem." END "\n");
        exit(EXIT_FAILURE);
    }
    struct sigaction sa;
    sa.sa_handler = handleAlarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Cashier: sigaction");
        exit(EXIT_FAILURE);
    }

    alarm(oppeningTime);

    while (!time_up) {
        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = -1;  // P
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("Cashier: semop P");
            continue;
        }
        msg_t receivedMsg;
        int msgType = receiveMessage(msgid, &receivedMsg);
        if (msgType == 1) {
            printf("VIP Client received: PID=%d\n", receivedMsg.pid);
        } else if (msgType == 2) {
            printf("Regular Client received: PID=%d\n", receivedMsg.pid);
        } else if (msgType == 0) {
            sleep(1);       // Dodaj opóźnienie
            sb.sem_op = 1;  // unlock sem
            if (semop(semid, &sb, 1) == -1) {
                perror("Cashier: semop V");
            }

            continue;
        }
        printf(BLUE
               "Cashier: Received client PID=%d, VIP=%ld, adultAge=%d, "
               "hasChild=%d, "
               "childAge=%d, client pool: %d, child pool: %d" END "\n",
               receivedMsg.pid, receivedMsg.mtype, receivedMsg.adultAge,
               receivedMsg.hasChild, receivedMsg.childAge, receivedMsg.poolId,
               receivedMsg.childPoolId);

        int can_enter = 1;
        char reason[MSG_SIZE] = "Enter allowed.";

        switch (receivedMsg.poolId) {
            case OLIMPIC:
                if (shdata->isFacilityClosed == 1) {
                    can_enter = 0;
                    strcpy(reason, "Building closed - water change.");
                }
                if (shdata->isOlimpicOpen == 0) {
                    can_enter = 0;
                    strcpy(reason, "Olimpic closed - can't enter.");
                }
                if (receivedMsg.adultAge < 18) {
                    can_enter = 0;
                    strcpy(reason,
                           "Only adults on Olimpic.");  // probably won't happen
                    // in randomization i dont think age <18 and Olimpic can be
                    // together, but just to make sure
                }
                if (shdata->olimpicCount >= MAX_CAPACITY_OLIMPIC) {
                    can_enter = 0;
                    strcpy(reason, "Olimpic is FULL.");
                }
                // when adult have child - he goes to recre so we need to check
                // if child can enter alongside with adult
                if (receivedMsg.hasChild) {
                    if (shdata->recreCount >= MAX_CAPACITY_RECRE) {
                        can_enter = 0;
                        strcpy(reason, "Recre for child is FULL.");
                    }
                    // we dont need to check for average, because it will only
                    // lower - child no older then 10 will enter this so its
                    // pointless to check
                }
                break;
            case RECRE:
                if (shdata->isFacilityClosed == 1) {
                    can_enter = 0;
                    strcpy(reason, "Building closed - water change.");
                }
                if (shdata->isRecreOpen == 0) {
                    can_enter = 0;
                    strcpy(reason, "Recre closed - can't enter.");
                }
                if (shdata->recreCount >= MAX_CAPACITY_RECRE) {
                    can_enter = 0;
                    strcpy(reason, "Recre is FULL.");
                } else {
                    // when adult is going to RECRE with child there needs to be
                    // 2 free slots
                    if (receivedMsg.hasChild) {
                        if (shdata->recreCount >= MAX_CAPACITY_RECRE - 1) {
                            can_enter = 0;
                            strcpy(reason, "Recre is FULL for (2).");
                        }
                        // when we calculate avg for duo we need to do it
                        // togheter
                        // because they will come simultaneously
                        double avg_age =
                            (shdata->recreSumAge + receivedMsg.adultAge +
                             receivedMsg.childAge) /
                            (double)(shdata->recreCount + 2);
                        if (avg_age > 40.0) {
                            can_enter = 0;
                            strcpy(reason, "Age average will be over 40.");
                        }
                    } else {
                        double avg_age =
                            (shdata->recreSumAge + receivedMsg.adultAge) /
                            (double)(shdata->recreCount + 1);
                        if (avg_age > 40.0) {
                            can_enter = 0;
                            strcpy(reason, "Age average will be over 40.");
                        }
                    }
                }
                break;
            case CHILD:
                if (shdata->isFacilityClosed == 1) {
                    can_enter = 0;
                    strcpy(reason, "Building closed - water change.");
                }
                if (shdata->isChildOpen == 0) {
                    can_enter = 0;
                    strcpy(reason, "Child closed - can't enter.");
                }
                // only childs and guardians, in randomizing clients we have
                // already made sure that they will be together
                if (receivedMsg.hasChild == 0) {
                    can_enter = 0;
                    strcpy(reason, "Only child with guardians allowed.");
                }
                if (shdata->childCount >= MAX_CAPACITY_CHILD - 1) {
                    can_enter = 0;
                    strcpy(reason, "Child is FULL.");
                }
                if (receivedMsg.childAge < 3 && receivedMsg.hasPampers != 1) {
                    can_enter = 0;
                    strcpy(reason, "Child without pampers.");
                }
                break;
            default:
                can_enter = 0;
                strcpy(reason, "No such pool.");
                break;
        }

        // update capacity
        if (can_enter) {
            pthread_mutex_lock(&shdata->mutex);  // lock access

            switch (receivedMsg.poolId) {
                case OLIMPIC:
                    shdata->olimpicCount++;
                    // if adult in olimpic have child, child will be in recre
                    if (receivedMsg.hasChild) {
                        shdata->recreCount++;
                        shdata->recreSumAge += receivedMsg.childAge;
                    }
                    break;
                case RECRE:
                    shdata->recreCount++;
                    shdata->recreSumAge += receivedMsg.adultAge;
                    // if adult in recre have child, child will be in recre
                    if (receivedMsg.hasChild) {
                        shdata->recreCount++;
                        shdata->recreSumAge += receivedMsg.childAge;
                    }

                    break;
                case CHILD:
                    shdata->childCount +=
                        2;  // +2 because guardian will also enter
                    break;
            }
            pthread_mutex_unlock(&shdata->mutex);  // unlock
        }

        msg_t response_msg;
        response_msg.mtype = receivedMsg.pid;
        response_msg.pid = getpid();  // PID adult
        response_msg.adultAge = receivedMsg.adultAge;
        response_msg.poolId = receivedMsg.poolId;
        response_msg.isVip = 0;
        response_msg.hasChild = receivedMsg.hasChild;
        response_msg.status = can_enter ? 1 : 0;
        if (can_enter) {
            snprintf(response_msg.text, MSG_SIZE,
                     "Enter allowed. Current status: Olimpic: %d/%d, Recre: "
                     "%d/%d, Child: %d/%d.",
                     shdata->olimpicCount, MAX_CAPACITY_OLIMPIC,
                     shdata->recreCount, MAX_CAPACITY_RECRE, shdata->childCount,
                     MAX_CAPACITY_CHILD);
        } else {
            snprintf(response_msg.text, MSG_SIZE,
                     "Refused: %s. Current status: Olimpic: %d/%d, Recre: "
                     "%d/%d, Child: %d/%d.",
                     reason, shdata->olimpicCount, MAX_CAPACITY_OLIMPIC,
                     shdata->recreCount, MAX_CAPACITY_RECRE, shdata->childCount,
                     MAX_CAPACITY_CHILD);
        }
        response_msg.text[MSG_SIZE - 1] = '\0';
        validateCounters(shdata);
        // send response

        if (msgsnd(msgid, &response_msg, sizeof(msg_t) - sizeof(long), 0) ==
            -1) {
            perror("Cashier: msgsnd answear");
            continue;
        } else {
            // printf(RED "HERE1." END "\n");

            printf(GREEN "Response sent to client PID=%d." END "\n",
                   response_msg.mtype);
        }

        //
        // printf(RED "HERE2." END "\n");

        if (!can_enter) {
            printf(YELLOW "Cashier: Refused: %s" END "\n", response_msg.text);
        }
        // quit critical section
        sb.sem_op = 1;  // V
        if (semop(semid, &sb, 1) == -1) {
            perror("Cashier: semop V");
            continue;
        }
        sleep(1);
    }
    printf("Cashier: Ending.\n");
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("Cashier: msgctl IPC_RMID");
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, RED "Cashier: problem with detach shmem." END "\n");
    }
    return 0;
}