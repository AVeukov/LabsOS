CC=gcc
CFLAGS=-Wall -Wextra -O2

all: pipe fifo_writer fifo_reader

pipe: pipe.c
	$(CC) $(CFLAGS) pipe.c -o pipe

fifo_writer: fifo_writer.c
	$(CC) $(CFLAGS) fifo_writer.c -o fifo_writer

fifo_reader: fifo_reader.c
	$(CC) $(CFLAGS) fifo_reader.c -o fifo_reader

clean:
	rm -f pipe fifo_writer fifo_reader lab6_fifo
