CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: scheduler

scheduler: main.c
	$(CC) $(CFLAGS) -o scheduler main.c

clean:
	rm -f scheduler *.out
