
#ifndef MSG_STRUCT_H
#define MSG_STRUCT_H
#include <sys/types.h>

#define MSG_QUEUE_KEY 0x1240
#define MSG_SIZE 256

// Definicja struktury komunikatu
typedef struct {
    long mtype;
    pid_t pid;

    int adultAge;
    int poolId;

    int hasChild;
    int childAge;
    int hasPampers;

    int status;      //  0=OK, -1=refused
    char text[128];  // additional info
} msg_t;
int create_message_queue();
int open_message_queue();
#endif