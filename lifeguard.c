#include "globals.h"
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Użycie: %s <poolId>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int poolId = atoi(argv[1]);  // 1=olimpic, 2=recre, 3=child

    printf("Hello from lifeguard on pool number %d \n", poolId);
    return 0;
}