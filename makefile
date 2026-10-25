
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_DEFAULT_SOURCE

COMMON = tftp.c udp.c file.c

all: client server

client: client.c $(COMMON)
	$(CC) $(CFLAGS) client.c $(COMMON) -o client

server: server.c $(COMMON)
	$(CC) $(CFLAGS) server.c $(COMMON) -o server

clean:
	rm -f client server *.o

.PHONY: all clean
