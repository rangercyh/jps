#ifndef __HEAP_H__
#define __HEAP_H__

typedef struct heap_s heap_t;

heap_t *
heap_new();

void
heap_init(heap_t *h, int (*cmp)(int, int, void *udata), void *udata);

void
heap_free(heap_t *h);

int
heap_insert(heap_t **h, int item);

int
heap_pop(heap_t *h);

int
heap_peek(heap_t *h);

void
heap_clear(heap_t *h);

int
heap_count(heap_t *h);

void
push_up(heap_t *h, int item);

void
push_down(heap_t *h, int item);

#ifdef DEBUG
void
heap_dump(heap_t *h);
#endif // DEBUG

#endif /* __HEAP_H__ */
