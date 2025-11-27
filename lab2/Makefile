CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -O2 -Wno-unused-variable -Wno-unused-parameter
TARGET = myls

all: $(TARGET)

$(TARGET): myls.c
	$(CC) $(CFLAGS) -o $(TARGET) myls.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
