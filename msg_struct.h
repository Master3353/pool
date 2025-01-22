
#ifndef MSG_STRUCT_H
#define MSG_STRUCT_H
#include <sys/types.h>

#define MSG_QUEUE_KEY 0x1240
#define MSG_SIZE 256

typedef struct {
    long mtype;
    pid_t pid;

    int adultAge;
    int poolId;
    int childPoolId;
    int isVip;  // 1-yes,0-no
    int hasChild;
    int childAge;
    int hasPampers;

    int status;      //  1=OK, 0=refused
    char text[128];  // additional info
} msg_t;

int create_message_queue();
int open_message_queue();
#endif