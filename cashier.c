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

    // while (1) {
    //     msg_t receivedMsg;

    //     if (msgrcv(msgid, &receivedMsg, sizeof(msg_t) - sizeof(long), 0, 0)
    //     ==
    //         -1) {
    //         if (errno == EINTR) continue;
    //         perror("Cashier: msgrcv error");
    //         break;
    //     }

    //     printf(
    //         "Cashier: Received client PID=%d, age=%d, VIP=%d, "
    //         "pool=%d\n",
    //         receivedMsg.clientPid, receivedMsg.age, receivedMsg.isVip,
    //         receivedMsg.targetPool);

    //     int can_enter = 1;
    //     char reason[MSG_SIZE] = "Enter allowed.";

    //     struct sembuf sb;
    //     sb.sem_num = 0;
    //     sb.sem_op = -1;  // P
    //     sb.sem_flg = 0;
    //     if (semop(semid, &sb, 1) == -1) {
    //         perror("Cashier: semop P");
    //         continue;
    //     }

    //     switch (receivedMsg.targetPool) {
    //         case OLIMPIC:
    //             if (receivedMsg.age < 18) {
    //                 can_enter = 0;
    //                 strcpy(reason,
    //                        "Dorosłe osoby tylko w basenie olimpijskim.");
    //             }
    //             if (shdata->olimpicCount >= MAX_CAPACITY_OLIMPIC) {
    //                 can_enter = 0;
    //                 strcpy(reason, "Basen olimpijski jest pełny.");
    //             }
    //             break;
    //         case RECRE:
    //             if (shdata->recreCount >= MAX_CAPACITY_RECRE) {
    //                 can_enter = 0;
    //                 strcpy(reason, "Basen rekreacyjny jest pełny.");
    //             } else {
    //                 double avg_age = (shdata->recreSumAge + receivedMsg.age)
    //                 /
    //                                  (double)(shdata->recrePeopleCount + 1);
    //                 if (avg_age > 40.0) {
    //                     can_enter = 0;
    //                     strcpy(reason, "Średnia wieku przekracza 40 lat.");
    //                 }
    //             }
    //             break;
    //         case CHILD:
    //             if (receivedMsg.age > 5) {
    //                 can_enter = 0;
    //                 strcpy(reason,
    //                        "Brodzik przeznaczony tylko dla dzieci do 5
    //                        lat.");
    //             }
    //             if (shdata->childCount >= MAX_CAPACITY_CHILD) {
    //                 can_enter = 0;
    //                 strcpy(reason, "Brodzik dziecięcy jest pełny.");
    //             }
    //             break;
    //         default:
    //             can_enter = 0;
    //             strcpy(reason, "Nieprawidłowy identyfikator basenu.");
    //             break;
    //     }

    //     // Aktualizacja pamięci dzielonej, jeśli klient może wejść
    //     if (can_enter) {
    //         switch (receivedMsg.targetPool) {
    //             case 1:
    //                 shdata->olimpicCount++;
    //                 break;
    //             case 2:
    //                 shdata->recreCount++;
    //                 shdata->recrePeopleCount++;
    //                 shdata->recreSumAge += receivedMsg.age;
    //                 break;
    //             case 3:
    //                 shdata->childCount++;
    //                 break;
    //         }
    //     }

    //     // Wyjście z sekcji krytycznej
    //     sb.sem_op = 1;  // V
    //     if (semop(semid, &sb, 1) == -1) {
    //         perror("Kasjer: semop V");
    //         continue;
    //     }

    //     // Przygotowanie odpowiedzi
    //     msg_t response_msg;
    //     response_msg.mtype =
    //         receivedMsg.clientPid;         // Typ odpowiadający PID klienta
    //     response_msg.clientPid = getpid();  // PID kasjera (opcjonalnie)
    //     response_msg.age = receivedMsg.age;
    //     response_msg.isVip = receivedMsg.isVip;
    //     response_msg.targetPool = receivedMsg.targetPool;
    //     response_msg.status = can_enter ? 0 : -1;
    //     strncpy(response_msg.text, reason, MSG_SIZE - 1);
    //     response_msg.text[MSG_SIZE - 1] =
    //         '\0';  // Bezpieczne zakończenie stringa

    //     // Wysyłanie odpowiedzi
    //     if (msgsnd(msgid, &response_msg, sizeof(msg_t) - sizeof(long), 0) ==
    //         -1) {
    //         perror("Kasjer: msgsnd odpowiedź");
    //         continue;
    //     }

    //     if (can_enter) {
    //         printf(
    //             "Kasjer: Wysłałem odpowiedź: Wejście do basenu
    //             dozwolone.\n");
    //     } else {
    //         printf("Kasjer: Wysłałem odpowiedź: %s\n", reason);
    //     }
    // }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Cashier: problem with detach shmem.\n");
    }
    return 0;
}