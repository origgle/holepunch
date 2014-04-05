
all: client server

util.o : util.c
	cc -c -o util.o util.c

set.o : set.c
	cc -c -o set.o set.c

server : server.c set.o util.o
	cc -o server server.c util.o set.o -pthread

client : client.c set.o util.o
	cc -o client client.c util.o set.o

clean:
	rm client server util.o set.o
