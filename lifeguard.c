#include "globals.h"

static volatile sig_atomic_t shouldClose = 0;
static volatile sig_atomic_t shouldOpen = 0;
volatile sig_atomic_t time_up = 0;

pthread_mutex_t fifoMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fifoCond = PTHREAD_COND_INITIALIZER;
pid_t pidQueue[100];
int queueStart = 0, queueEnd = 0;

void enqueuePid(pid_t pid) {
    pthread_mutex_lock(&fifoMutex);
    pidQueue[queueEnd] = pid;
    queueEnd = (queueEnd + 1) % 100;
    pthread_cond_signal(&fifoCond);
    pthread_mutex_unlock(&fifoMutex);
}

// get pid from queue
pid_t dequeuePid() {
    pthread_mutex_lock(&fifoMutex);
    while (queueStart == queueEnd) {
        pthread_cond_wait(&fifoCond, &fifoMutex);
    }
    pid_t pid = pidQueue[queueStart];
    queueStart = (queueStart + 1) % 100;
    pthread_mutex_unlock(&fifoMutex);
    return pid;
}
void* fifoReaderThread(void* arg) {
    const char* fifoName = (const char*)arg;
    int fd = open(fifoName, O_RDONLY);
    if (fd == -1) {
        perror("Lifeguard: Error opening FIFO");
        pthread_exit(NULL);
    }

    char buffer[32];
    while (1) {
        ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            pid_t pid = atoi(buffer);
            if (pid > 0) {
                enqueuePid(pid);
            }
        }
        // usleep(100000);  // wait 100ms so cpu won't explode
    }

    close(fd);
    pthread_exit(NULL);
}
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
        fprintf(stderr, "Usage: %s <poolId> <oppeningTime>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int poolId = atoi(argv[1]);
    int oppeningTime = atoi(argv[2]);

    struct sigaction sa_close, sa_open, sa_alarm;
    sa_close.sa_handler = sigusr1Handler;
    sigemptyset(&sa_close.sa_mask);
    sa_close.sa_flags = 0;
    sigaction(SIGUSR1, &sa_close, NULL);

    sa_open.sa_handler = sigusr2Handler;
    sigemptyset(&sa_open.sa_mask);
    sa_open.sa_flags = 0;
    sigaction(SIGUSR2, &sa_open, NULL);

    sa_alarm.sa_handler = handleAlarm;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = 0;
    alarm(oppeningTime);

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
    printf(RED "Lifeguard (%d)" END "\n", poolId);

    switch (poolId) {
        case 1:
            fifoName = "./fifo_olimpic";
            break;
        case 2:
            fifoName = "./fifo_recre";
            break;
        case 3:
            fifoName = "./fifo_child";
            break;
        default:
            fprintf(stderr, "Lifeguard: Invalid pool ID\n");
            exit(EXIT_FAILURE);
    }
    pthread_t fifoThread;
    if (pthread_create(&fifoThread, NULL, fifoReaderThread, (void*)fifoName) !=
        0) {
        perror("Lifeguard: Error creating FIFO reader thread");
        exit(EXIT_FAILURE);
    }

    while (!time_up) {
        // sleep(1);  // wait 1 sec so cpu won't explode
        if (shouldClose) {
            printf("Lifeguard: Closing pool %d\n", poolId);

            // Zamykanie basenu
            pthread_mutex_lock(&shdata->mutex);
            if (poolId == 1) shdata->isOlimpicOpen = 0;
            if (poolId == 2) shdata->isRecreOpen = 0;
            if (poolId == 3) shdata->isChildOpen = 0;
            pthread_mutex_unlock(&shdata->mutex);

            while (queueStart != queueEnd) {
                pid_t pid = dequeuePid();
                // attempting to terminate clients - when PID doesn;t exists
                // omit it
                if (kill(pid, SIGTERM) == -1) {
                    if (errno == ESRCH) {
                        printf("Lifeguard: PID=%d does not exist.\n", pid);
                    } else {
                        perror("Lifeguard: Error sending SIGTERM");
                    }
                }
            }

            printf(RED "Ended terminating" END "\n ");

            shouldClose = 0;  // reset flag
        }

        if (shouldOpen) {
            printf("Lifeguard: Opening pool %d\n", poolId);

            // open pool
            pthread_mutex_lock(&shdata->mutex);
            if (poolId == OLIMPIC) shdata->isOlimpicOpen = 1;
            if (poolId == RECRE) shdata->isRecreOpen = 1;
            if (poolId == CHILD) shdata->isChildOpen = 1;
            pthread_mutex_unlock(&shdata->mutex);

            shouldOpen = 0;  // reset flag
        }
    }

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Lifeguard: problem with detach shmem.\n");
    }
    pthread_cancel(fifoThread);
    pthread_join(fifoThread, NULL);
    return 0;
}