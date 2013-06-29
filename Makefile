CFLAGS=-Wall -Wextra -g
LDLIBS=-lzmq

all: polecat

polecat: fifo.o cleanup.o

fifo.o: fifo.h

cleanup.o: cleanup.h

clean: rm -f *.o

.PHONY: all clean
