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

#define SHM_KEY 0x1234

#define OLIMPIC 1
#define RECRE 2
#define CHILD 3
#define TIME_OPEN  // not sure yet how to implement this
#define TIME_CLOSED
#define MAX_CAPACITY_OLIMPIC 30
#define MAX_CAPACITY_RECRE 60
#define MAX_CAPACITY_CHILD 40
// 1 - olimpic, 2 - recre, 3 - child

typedef struct {
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

int createSharedMemory();
int getSharedMemory();
SharedMemory* attachSharedMemory(int shmid);
int detachSharedMemory(SharedMemory* shdata);
int destroySharedMemory(int shmid);
void initializeSharedData(SharedMemory* shdata);

void checkInput();

#endif