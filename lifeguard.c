#include "globals.h"

static volatile sig_atomic_t shouldClose = 0;
static volatile sig_atomic_t shouldOpen = 0;
volatile sig_atomic_t time_up = 0;

void sigusr1Handler(int sig) {
    if (sig == SIGUSR1) {
        shouldClose = 1;  // closing pool
    }
}

void sigusr2Handler(int sig) {
    if (sig == SIGUSR2) {
        shouldOpen = 1;  // opening pool
    }
}
void handleAlarm(int sig) { time_up = 1; }
int main(int argc, char* argv[]) {
    // takes on which pool he is staying and cashier pid for comunication
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <poolId> <cashierPid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int poolId = atoi(argv[1]);
    int cashierPid = (pid_t)atoi(argv[2]);

    struct sigaction sa_close, sa_open, sa_alarm;
    sa_close.sa_handler = sigusr1Handler;
    sigemptyset(&sa_close.sa_mask);
    sa_close.sa_flags = 0;
    sigaction(SIGUSR1, &sa_close, NULL);

    sa_open.sa_handler = sigusr2Handler;
    sigemptyset(&sa_open.sa_mask);
    sa_open.sa_flags = 0;

    sa_alarm.sa_handler = handleAlarm;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = 0;
    alarm(50);
    sigaction(SIGUSR2, &sa_open, NULL);

    int shmid = createSharedMemory();
    if (shmid == -1) {
        fprintf(stderr, "Lifeguard: Probelm with getting shmem.\n");
        exit(EXIT_FAILURE);
    }

    SharedMemory* shdata = attachSharedMemory(shmid);
    if (shdata == (void*)-1) {
        fprintf(stderr, "Lifeguard: problem with attach shmem.\n");
        exit(EXIT_FAILURE);
    }
    const char* fifoName;
    switch (poolId) {
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
            fprintf(stderr, "Lifeguard: Invalid pool ID\n");
            exit(EXIT_FAILURE);
    }

    int fd = open(fifoName, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("Error reading FIFO");
        return;
    }
    // ;

    // printf("Lifeguard: i have shmem! olimpicCount: %d\n",
    // shdata->olimpicCount);
    while (!time_up) {
        if (shouldClose) {
            printf("Lifeguard: Closing pool %d\n", poolId);

            // Zamykanie basenu
            pthread_mutex_lock(&shdata->mutex);
            if (poolId == OLIMPIC) shdata->isOlimpicOpen = 0;
            if (poolId == RECRE) shdata->isRecreOpen = 0;
            if (poolId == CHILD) shdata->isChildOpen = 0;
            pthread_mutex_unlock(&shdata->mutex);

            char buffer[32];
            while (read(fd, buffer, sizeof(buffer)) > 0) {
                pid_t pid = atoi(buffer);
                if (pid > 0) {
                    printf("Lifeguard: Attempting to terminate PID=%d\n", pid);
                    if (kill(pid, SIGTERM) == -1) {
                        if (errno == ESRCH) {
                            // printf(
                            //     "Lifeguard: PID=%d does not exist,
                            //     skipping\n",
                            //    pid);
                        } else {
                            perror("Lifeguard: Error sending SIGTERM");
                        }
                    } else {
                        printf(RED
                               "Lifeguard: Successfully terminated PID=%d" END
                               "\n ",
                               pid);
                    }
                }
            }
            // close(fd);

            shouldClose = 0;  // Resetuj flagę
        }

        if (shouldOpen) {
            printf("Lifeguard: Opening pool %d\n", poolId);

            // Otwarcie basenu
            pthread_mutex_lock(&shdata->mutex);
            if (poolId == OLIMPIC) shdata->isOlimpicOpen = 1;
            if (poolId == RECRE) shdata->isRecreOpen = 1;
            if (poolId == CHILD) shdata->isChildOpen = 1;
            pthread_mutex_unlock(&shdata->mutex);

            shouldOpen = 0;  // Resetuj flagę
        }

        sleep(1);  // Uniknięcie obciążenia CPU
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Lifeguard: problem with detach shmem.\n");
    }
    close(fd);
    return 0;
}