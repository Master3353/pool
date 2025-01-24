#include "globals.h"
// variable
// static volatile sig_atomic_t shouldClose = 0;

// void sigusr1Handler(int sig) {
//     if (sig == SIGUSR1) {
//         shouldClose = 1;
//     }
// }

int main(int argc, char* argv[]) {
    // takes on which pool he is staying and cashier pid for comunication
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <poolId> <cashierPid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int poolId = atoi(argv[1]);
    int cashierPid = (pid_t)atoi(argv[2]);

    // struct sigaction sa;
    // sa.sa_handler = sigusr1Handler;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = 0;
    // sigaction(SIGUSR1, &sa, NULL);

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

    // printf("Lifeguard: i have shmem! olimpicCount: %d\n",
    // shdata->olimpicCount);

    if (detachSharedMemory(shdata) == -1) {
        fprintf(stderr, "Lifeguard: problem with detach shmem.\n");
    }

    return 0;
}