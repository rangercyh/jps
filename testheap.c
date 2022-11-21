#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "heap.h"

// min
static int compare(int a, int b, void *u) {
    return b - a;
}

void main() {
    int vals[10] = { 9, 2, 5, 7, 4, 6, 3, 8, 1, 0 };
    heap_t *hp = heap_new();
    heap_init(hp, compare, NULL);
    for (int i = 0; i < 5; i++) {
        heap_insert(&hp, vals[i]);
        heap_dump(hp);
    }
    for (int i = 0; i < 5; i++) {
        printf("%d\n", heap_pop(hp));
        heap_dump(hp);
    }
    heap_free(hp);
}
