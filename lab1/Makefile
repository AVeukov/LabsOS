# Makefile для lab1
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2

TARGETS = mycat mygrep

.PHONY: all clean

all: $(TARGETS)

mycat: mycat.c
	$(CC) $(CFLAGS) -o $@ mycat.c

mygrep: mygrep.c
	$(CC) $(CFLAGS) -o $@ mygrep.c

clean:
	rm -f $(TARGETS) *.o
