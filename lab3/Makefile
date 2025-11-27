# Makefile
CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -pedantic
TARGET = lab3
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
