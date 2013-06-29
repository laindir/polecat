/*

Copyright 2012 Carl D Hamann

This file is part of polecat.

polecat is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

polecat is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with polecat.  If not, see <http://www.gnu.org/licenses/>.

*/

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

	return startindex <= endindex ? endindex - startindex : fifo->size - startindex;
}
