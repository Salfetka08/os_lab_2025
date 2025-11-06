# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread

LIB_OBJ = mult_modulo.o

all: client server

mult_modulo.o: mult_modulo.c mult_modulo.h
	$(CC) $(CFLAGS) -c mult_modulo.c

client: client.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o client client.c $(LIB_OBJ)

server: server.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o server server.c $(LIB_OBJ)

clean:
	rm -f client server $(LIB_OBJ)

run-server1:
	./server --port 20001 --tnum 4 --bind 0.0.0.0

run-server2:
	./server --port 20002 --tnum 4 --bind 0.0.0.0

run-server-local:
	./server --port 20001 --tnum 4 --bind 127.0.0.1

run-client:
	./client --k 10 --mod 1000 --servers servers.txt

.PHONY: all clean run-server1 run-server2 run-server-local run-client