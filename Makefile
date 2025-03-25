CC     = gcc
CFLAGS = -Wall -Wextra -pthread

all: server client

server: chat.c
	$(CC) $(CFLAGS) -o hw3server chat.c -D_SERVER

client: hw3_323884692_208826040.c
	$(CC) $(CFLAGS) -o hw3client chat.c -D_CLIENT

clean:
	rm -f server client
