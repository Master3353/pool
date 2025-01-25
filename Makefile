CFLAGS = -Wall -Wextra -pthread

all: pool cashier lifeguard client

pool: pool.c globals.c msg_struct.c
	gcc $(CFLAGS) -o pool pool.c globals.c msg_struct.c
cashier: cashier.c globals.c msg_struct.c
	gcc $(CFLAGS) -o cashier cashier.c globals.c msg_struct.c
lifeguard: lifeguard.c globals.c msg_struct.c
	gcc $(CFLAGS) -o lifeguard lifeguard.c globals.c msg_struct.c
client: client.c globals.c msg_struct.c
	gcc $(CFLAGS) -o client client.c globals.c msg_struct.c 
			
clean:
	rm -f pool  cashier lifeguard client fifo_child fifo_olimpic fifo_recre
