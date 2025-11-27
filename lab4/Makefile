CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
TARGET = mychmod

all: $(TARGET)

$(TARGET): mychmod.c
	$(CC) $(CFLAGS) -o $(TARGET) mychmod.c

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
