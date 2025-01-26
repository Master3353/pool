#ifndef GLOBALS_H
#define GLOBALS_H

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define LIFEGUARD_FIFO "/tmp/lifeguard_fifo"
#define SHM_KEY 0x1234
#define SEM_KEY 0x5678
#define FIFO_SEM_KEY 0x5000
#define SEM_COUNT 1

#define OLIMPIC 1
#define RECRE 2
#define CHILD 3
#define TIME_OPEN  // not sure yet how to implement this
#define TIME_CLOSED
#define MAX_CAPACITY_OLIMPIC 10
#define MAX_CAPACITY_RECRE 20
#define MAX_CAPACITY_CHILD 20
// 1 - olimpic, 2 - recre, 3 - child

// define colors for better output
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define WHITE "\033[0;37m"
#define END "\033[0m"

typedef struct {
    pthread_mutex_t mutex;

    // current population in different pools
    int olimpicCount;
    int recreCount;
    int childCount;
    // tracking if mean age in recre <=40
    int recreSumAge;
    // tracking if specific pool is open
    int isOlimpicOpen;
    int isRecreOpen;
    int isChildOpen;
    // tracking if building is open
    int isFacilityClosed;  // 0 - open, 1 - closed
} SharedMemory;

// functions for shared mem
int createSharedMemory();
int getSharedMemory();
SharedMemory* attachSharedMemory(int shmid);
int detachSharedMemory(SharedMemory* shdata);
int destroySharedMemory(int shmid);
void initializeSharedData(SharedMemory* shdata);

// functions for fifo
void createFifo(const char* fifoName);
void addPidToFifo(const char* fifoName, pid_t pid, int fifoSemid);
int initFifoSemaphore();
void fifoSemaphoreUnlock(int semid);
void fifoSemaphoreLock(int semid);

void checkInput();

#endif