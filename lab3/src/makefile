CC = gcc
CFLAGS = -Wall -I.

all: sequential_min_max parallel_min_max exec_program

sequential_min_max: utils.o find_min_max.o
    $(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

parallel_min_max: utils.o find_min_max.o
    $(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

exec_program: src/exec_program.c
    $(CC) $(CFLAGS) -o exec_program src/exec_program.c

utils.o: utils.c utils.h
    $(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h
    $(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean:
    rm -f utils.o find_min_max.o sequential_min_max parallel_min_max exec_program

.PHONY: all clean