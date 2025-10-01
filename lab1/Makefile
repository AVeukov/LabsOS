CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDFLAGS =
TARGETS = mycat mygrep

all: $(TARGETS)

mycat: mycat.c
	$(CC) $(CFLAGS) -o $@ mycat.c $(LDFLAGS)

mygrep: mygrep.c
	$(CC) $(CFLAGS) -o $@ mygrep.c $(LDFLAGS) -lrt

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean
