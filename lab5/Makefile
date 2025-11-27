CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-result -O2
TARGET = archiver

all: $(TARGET)

$(TARGET): archiver.c
	$(CC) $(CFLAGS) -o $(TARGET) archiver.c

clean:
	rm -f $(TARGET) *.o *.tmp*

.PHONY: all clean
