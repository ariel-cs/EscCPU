CC = gcc
CFLAGS = -Wall -Wextra

all: scheduler

scheduler: main.c
	$(CC) $(CFLAGS) -o scheduler main.c

clean:
	rm -f scheduler *.out
