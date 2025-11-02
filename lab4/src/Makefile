CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pthread

parallel_sum: parallel_sum.c utils.c sum.c
	$(CC) $(CFLAGS) -o parallel_sum parallel_sum.c utils.c sum.c

clean:
	rm -f parallel_sum

run:
	./parallel_sum --threads_num 4 --seed 123 --array_size 1000000