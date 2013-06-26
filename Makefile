CFLAGS=-Wall -Wextra -pedantic -pg -g -std=c89
LDLIBS=-lzmq

all: polecat

foo: coroutine.o

polecat: fifo.o

fifo.o: fifo.h

coroutine.o: coroutine.h

clean: rm -f *.o

.PHONY: all clean
