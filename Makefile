CC=gcc
CFLAGS=-g -O1 -Wall
LDLIBS=-lpthread

all: client server admin

client: client.c csapp.h csapp.c
server: server.c csapp.h csapp.c
admin: admin.c csapp.h csapp.c

clean:
	rm -f *.o *~ *.exe client server admin csapp.o

