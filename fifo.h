#ifndef FIFO_H
#define FIFO_H

struct fifo
{
	int produced;
	int consumed;
	int size;
	char buffer[1];
};

struct fifo *
fifo_init(int size);

void
fifo_destroy(struct fifo *f);

int
is_full(struct fifo *fifo);

int
is_empty(struct fifo *fifo);

int
produce_index(struct fifo *fifo);

int
consume_index(struct fifo *fifo);

int
space_left(struct fifo *fifo);

int
space_filled(struct fifo *fifo);

#endif
