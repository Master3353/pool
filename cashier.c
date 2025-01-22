#include "globals.h"
#include "msg_struct.h"

int init_semaphore() {
    int semid = semget(SEM_KEY, SEM_COUNT, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("Cashier: semget error");
        exit(EXIT_FAILURE);
    }

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;

    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("Cashier: semctl error");
        exit(EXIT_FAILURE);
    }

    return semid;
}

int main(void) {
    int msgid = create_message_queue();
    int semid = init_semaphore();
    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Cashier: Probelm with getting shmem.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory *shdata = attachSharedMemory(shmid);
    if (shdata == (void *)-1) {
        fprintf(stderr, "Cashier: problem with attach shmem.\n");
        exit(EXIT_FAILURE);
    }
    printf("Hello from cashier!\n");

    while (1) {
        msg_t receivedMsg;

        if (msgrcv(msgid, &receivedMsg, sizeof(msg_t) - sizeof(long), 0, 0) ==
            -1) {
            if (errno == EINTR) continue;
            perror("Cashier: msgrcv error");
            break;
        }

        printf(
            "Cashier: Received client PID=%d, adultAge=%d, hasChild=%d, "
            "childAge=%d\n",
            receivedMsg.pid, receivedMsg.adultAge, receivedMsg.hasChild,
            receivedMsg.childAge);
        int can_enter = 1;
        char reason[MSG_SIZE] = "Enter allowed.";

        struct sembuf sb;
        sb.sem_num = 0;
        sb.sem_op = -1;  // P
        sb.sem_flg = 0;
        if (semop(semid, &sb, 1) == -1) {
            perror("Cashier: semop P");
            continue;
        }

        switch (receivedMsg.poolId) {
            case OLIMPIC:
                if (shdata->isFacilityClosed == 1) {
                    can_enter = 0;
                    strcpy(reason, "Building closed - water change.");
                    break;
                }
                if (shdata->isChildOpen == 0) {
                    can_enter = 0;
                    strcpy(reason, "Olimpic closed - can't enter.");
                    break;
                }
                if (receivedMsg.adultAge < 18) {
                    can_enter = 0;
                    strcpy(reason,
                           "Only adults on Olimpic.");  // probably won't happen
                    // in randomization i dont think age <18 and Olimpic can be
                    // together
                    break;
                }
                if (shdata->olimpicCount >= MAX_CAPACITY_OLIMPIC) {
                    can_enter = 0;
                    strcpy(reason, "Olimpic is FULL.");
                    break;
                }
                // when adult have child - he goes to recre so we need to check
                // if child can enter alongside with adult
                if (receivedMsg.hasChild) {
                    if (shdata->recreCount >= MAX_CAPACITY_RECRE) {
                        can_enter = 0;
                        strcpy(reason, "Recre for child is FULL.");
                        break;
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
                    break;
                }
                if (shdata->isRecreOpen == 0) {
                    can_enter = 0;
                    strcpy(reason, "Olimpic closed - can't enter.");
                    break;
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
                            strcpy(reason, "Recre is FULL.");
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
                    break;
                }
                if (shdata->isChildOpen == 0) {
                    can_enter = 0;
                    strcpy(reason, "Olimpic closed - can't enter.");
                    break;
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
        }

        // Wyjście z sekcji krytycznej
        sb.sem_op = 1;  // V
        if (semop(semid, &sb, 1) == -1) {
            perror("Cashier: semop V");
            continue;
        }

        // we nned
        msg_t response_msg;
        response_msg.status = can_enter ? 1 : 0;
        strncpy(response_msg.text, reason, MSG_SIZE - 1);
        response_msg.text[MSG_SIZE - 1] = '\0';

        // Wysyłanie odpowiedzi
        if (msgsnd(msgid, &response_msg, sizeof(msg_t) - sizeof(long), 0) ==
            -1) {
            perror("Cashier: msgsnd answear");
            continue;
        }

        if (can_enter) {
            printf("Cashier: Please enter!.\n");
        } else {
            printf("Cashier: Refused: %s\n", reason);
        }
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Cashier: problem with detach shmem.\n");
    }
    return 0;
}