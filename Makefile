CFLAGS=-Wall -Wextra -g
LDLIBS=-lzmq
RONNFLAGS=--manual='User Commands' --organization=laindir

all: polecat

polecat: fifo.o cleanup.o

fifo.o: fifo.h

cleanup.o: cleanup.h

%: %.ronn
	ronn --roff $(RONNFLAGS) $<

%.html: %.ronn
	ronn --html $(RONNFLAGS) $<

clean: rm -f *.o polecat

.PHONY: all clean
