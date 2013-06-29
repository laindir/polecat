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
