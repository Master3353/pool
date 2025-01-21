#include "msg_struct.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

int create_message_queue() {
    int msgid = msgget(MSG_QUEUE_KEY, IPC_CREAT | 0600);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    return msgid;
}
int open_message_queue() {
    int msgid = msgget(MSG_QUEUE_KEY, 0600);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    return msgid;
}