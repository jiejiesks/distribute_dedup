CC=gcc
CFLAGS=-pthread

all: client server

client: client.c md5c.c
	$(CC) -o client client.c md5c.c $(CFLAGS) 

server: server.c rw_hash.c
	$(CC) -o server server.c rw_hash.c $(CFLAGS)

clean:
	rm -f client server fingerprint_table delpath