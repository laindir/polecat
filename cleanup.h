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

#ifndef CLEANUP_H
#define CLEANUP_H

enum limits
{
	max_cleanup_items = 64
};

struct cleanup_stack *
cleanup_create(unsigned int size);

int
cleanup_push_r(struct cleanup_stack *stack, void (*func)(void *), void *data);

struct cleanup_item *
cleanup_pop_r(struct cleanup_stack *stack);

void
cleanup_restore_r(void *data);

void
cleanup_save_r(struct cleanup_stack *stack);

void
cleanup_item_run(struct cleanup_item *item);

void
cleanup_rewind_r(struct cleanup_stack *stack);

int
cleanup_push(void (*func)(void *), void *data);

struct cleanup_item *
cleanup_pop(void);

void
cleanup_rewind(void);

void
cleanup_save(void);
#endif
