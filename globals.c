#include "globals.h"

void checkInput() {
    if (MAX_CAPACITY_CHILD < 2) {
        fprintf(stderr, "Child pool needs to have at least 2 spots.\n");
        exit(1);
    }
    if (MAX_CAPACITY_OLIMPIC < 1) {
        fprintf(stderr, "Olimpic pool needs to have at least 1 spot.\n");
        exit(2);
    }
    if (MAX_CAPACITY_RECRE < 1) {
        fprintf(stderr, "Recreational pool needs to have at least 1 spot.\n");
        exit(3);
    }
}