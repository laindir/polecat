#include <stdlib.h>
#include "cleanup.h"

struct cleanup_item
{
	void (*func)(void *);
	void *data;
};

struct cleanup_stack
{
	struct cleanup_item *p;
	unsigned int size;
	struct cleanup_item *items;
};

static struct cleanup_item cleanup_stack[max_cleanup_items];
static struct cleanup_stack static_stack = {(struct cleanup_item *)&cleanup_stack, max_cleanup_items, (struct cleanup_item *)&cleanup_stack};

struct cleanup_stack *
cleanup_create(unsigned int size)
{
	struct cleanup_stack *s = malloc(sizeof(struct cleanup_stack) + size * sizeof(struct cleanup_item));
	if(s)
	{
		s->items = (struct cleanup_item *)(s + 1);
		s->p = (struct cleanup_item *)(s + 1);
		s->size = size;
	}

	return s;
}

void
cleanup_destroy(struct cleanup_stack *stack)
{
	free(stack);
}

int
cleanup_push_r(struct cleanup_stack *stack, void (*func)(void *), void *data)
{
	if(stack->p < (stack->items + stack->size))
	{
		stack->p->func = func;
		stack->p->data = data;
		stack->p++;
		return 0;
	}
	else
	{
		return -1;
	}
}

struct cleanup_item *
cleanup_pop_r(struct cleanup_stack *stack)
{
	if(stack->p > stack->items)
	{
		stack->p--;
		return stack->p;
	}
	else
	{
		return NULL;
	}
}

void
cleanup_restore_r(void *data)
{
	struct cleanup_stack *stack = (struct cleanup_stack *)data;
	struct cleanup_item *items = stack->items;
	stack->items = (stack->items)->data;
	stack->size += (items - stack->items);
	stack->p = items;
}

void
cleanup_save_r(struct cleanup_stack *stack)
{
	struct cleanup_item *old_items = stack->items;
	stack->items = stack->p;
	stack->size -= (stack->items - old_items);
	cleanup_push_r(stack, NULL, old_items);
	cleanup_push_r(stack, cleanup_restore_r, stack);
}

void
cleanup_item_run(struct cleanup_item *item)
{
	item->func(item->data);
}

void
cleanup_rewind_r(struct cleanup_stack *stack)
{
	struct cleanup_item *items = stack->items;

	while(stack->p > items)
	{
		stack->p--;
		cleanup_item_run(stack->p);
	}
}

int
cleanup_push(void (*func)(void *), void *data)
{
	return cleanup_push_r(&static_stack, func, data);
}

struct cleanup_item *
cleanup_pop(void)
{
	return cleanup_pop_r(&static_stack);
}

void
cleanup_rewind(void)
{
	cleanup_rewind_r(&static_stack);
}

void
cleanup_save(void)
{
	cleanup_save_r(&static_stack);
}
