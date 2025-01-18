CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: pool

pool: pool.c globals.c
	$(CC) $(CFLAGS) -o pool pool.c globals.c
	
clean:
	rm -f pool