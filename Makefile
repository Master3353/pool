CFLAGS = -Wall -Wextra -pthread

all: pool cashier lifeguard client

pool: pool.c globals.c
	gcc $(CFLAGS) -o pool pool.c globals.c
cashier: cashier.c globals.c
	gcc $(CFLAGS) -o cashier cashier.c globals.c
lifeguard: lifeguard.c globals.c
	gcc $(CFLAGS) -o lifeguard lifeguard.c globals.c
client: client.c globals.c
	gcc $(CFLAGS) -o client client.c globals.c
			
clean:
	rm -f pool  cashier lifeguard client
	ipcrm -M 0x1234