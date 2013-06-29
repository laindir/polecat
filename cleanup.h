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
