#include <stdlib.h>
#include "fifo.h"

struct fifo *
fifo_init(int size)
{
	struct fifo *f;

	f = calloc(sizeof(struct fifo) + size - 1, 1);

	if(f)
	{
		f->size = size;
	}

	return f;
}

void
fifo_destroy(struct fifo *f)
{
	free(f);
}

int
is_full(struct fifo *fifo)
{
	return (fifo->produced - fifo->consumed) >= fifo->size;
}

int
is_empty(struct fifo *fifo)
{
	return (fifo->produced - fifo->consumed) <= 0;
}

int
produce_index(struct fifo *fifo)
{
	return fifo->produced % fifo->size;
}

int
consume_index(struct fifo *fifo)
{
	return fifo->consumed % fifo->size;
}

int
space_left(struct fifo *fifo)
{
	int startindex = produce_index(fifo);
	int endindex = consume_index(fifo);

	return startindex < endindex ? endindex - startindex : fifo->size - startindex;
}

int
space_filled(struct fifo *fifo)
{
	int startindex = consume_index(fifo);
	int endindex = produce_index(fifo);

	return startindex < endindex ? endindex - startindex : fifo->size - startindex;
}
