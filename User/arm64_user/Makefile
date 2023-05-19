CC=aarch64-linux-gnu-gcc
CFLAGS=-pthread


all: client server

client: client_HM.c md5c.c
	$(CC) -static -o client_HM client_HM.c md5c.c $(CFLAGS)

server: server_HM.c rw_hash.c
	$(CC) -static -o server_HM server_HM.c rw_hash.c $(CFLAGS)

clean:
	rm -f client_HM server_HM fingerprint_table delpath