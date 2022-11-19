#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "heap.h"

#define DEFAULT_CAP 128

struct heap_s
{
    /* size of array */
    unsigned int size;
    /* items within heap */
    unsigned int count;
    /*  user data */
    void *udata;
    int (*cmp)(int, int, void *);
    int array[];
};

heap_t *
heap_new() {
    heap_t *h = (heap_t *)malloc(sizeof(heap_t) + DEFAULT_CAP * sizeof(int));
    if (!h) return NULL;
    h->size = DEFAULT_CAP;
    h->count = 0;
    h->udata = NULL;
    h->cmp = NULL;
    memset(h->array, 0, h->size * sizeof(h->array[0]));
    return h;
}

void
heap_init(heap_t *h, int (*cmp)(int, int, void *udata), void *udata) {
    h->cmp = cmp;
    h->udata = udata;
}

void
heap_free(heap_t *h) {
    free(h);
}

static int __left(const int idx) {
    return idx * 2 + 1;
}

static int __right(const int idx) {
    return idx * 2 + 2;
}

static int __parent(const int idx) {
    return (idx - 1) / 2;
}

static void __swap(heap_t * h, const int i1, const int i2) {
    int tmp = h->array[i1];
    h->array[i1] = h->array[i2];
    h->array[i2] = tmp;
}

static void __pushdown(heap_t *h, unsigned int idx) {
    while (1) {
        unsigned int lt = __left(idx);
        unsigned int rt = __right(idx);
        unsigned int c;
        if (rt >= h->count) {
            if (lt >= h->count) return;
            c = lt;
        } else if (h->cmp(h->array[lt], h->array[rt], h->udata) < 0) {
            c = rt;
        } else {
            c = lt;
        }
        if (h->cmp(h->array[idx], h->array[c], h->udata) < 0) {
            __swap(h, idx, c);
            idx = c;
        } else return;
    }
}

static void __pushup(heap_t *h, unsigned int idx) {
    while (idx > 0) {
        int p = __parent(idx);
        if (h->cmp(h->array[idx], h->array[p], h->udata) < 0) {
            return;
        } else {
            __swap(h, idx, p);
        }
        idx = p;
    }
}

static heap_t * __ensure_size(heap_t *h) {
    if (h->count < h->size) return h;
    h->size *= 2;
    return realloc(h, sizeof(heap_t) + h->size * sizeof(h->array[0]));
}

static int __item_get_idx(heap_t *h, int item) {
    unsigned int idx;
    for (idx = 0; idx < h->count; idx++)
        if (h->array[idx] == item)
            return idx;
    return -1;
}

int
heap_insert(heap_t **h, int item) {
    *h = __ensure_size(*h);
    if (*h == NULL) return -1;
    (*h)->array[(*h)->count] = item;
    __pushup(*h, (*h)->count++);
    return 0;
}

int
heap_pop(heap_t *h) {
    if (h->count <= 0) return -1;
    int item = h->array[0];
    h->array[0] = h->array[h->count - 1];
    h->count--;
    if (h->count > 1) {
        __pushdown(h, 0);
    }
    return item;
}

int
heap_peek(heap_t *h) {
    if (h->count <= 0) return -1;
    return h->array[0];
}

void
heap_clear(heap_t *h) {
    h->count = 0;
    memset(h->array, 0, h->size * sizeof(h->array[0]));
}

int
heap_count(heap_t *h) {
    return h->count;
}

void
push_up(heap_t *h, int item) {
    int idx = __item_get_idx(h, item);
    if (idx == -1) return;
    __pushup(h, idx);
}

void
push_down(heap_t *h, int item) {
    int idx = __item_get_idx(h, item);
    if (idx == -1) return;
    __pushdown(h, idx);
}

#ifdef DEBUG
void
heap_dump(heap_t *h) {
    unsigned int idx;
    printf("===== head %p, count:%d, size:%d\n", h, h->count, h->size);
    for (idx = 0; idx < h->count; idx++)
        printf("%d ", h->array[idx]);
    printf("\n");
}
#endif // DEBUG
