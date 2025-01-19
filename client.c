#include "globals.h"
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Użycie: %s <age> <poolId>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int age = atoi(argv[1]);
    int poolId = atoi(argv[2]);

    printf("Hello from client aged %d on pool %d \n", age, poolId);
}