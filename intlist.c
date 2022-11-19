#include "intlist.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct int_list {
    // Stores a fixed-size buffer in advance to avoid requiring
    // a heap allocation until we run out of space.
    int fixed[il_fixed_cap];

    // Points to the buffer used by the list. Initially this will
    // point to 'fixed'.
    int* data;

    // Stores how many integer fields each element has.
    int num_fields;

    // Stores the number of elements in the list.
    int num;

    // Stores the capacity of the array.
    int cap;

    // Stores an index to the free element or -1 if the free list
    // is empty.
    int free_element;
};

IntList * il_create(int num_fields) {
    IntList *il = (IntList *)malloc(sizeof(IntList));
    il->data = il->fixed;
    il->num = 0;
    il->cap = il_fixed_cap;
    il->num_fields = num_fields;
    il->free_element = -1;
    return il;
}

void il_destroy(IntList* il) {
    // Free the buffer only if it was heap allocated.
    if (il->data != il->fixed) {
        free(il->data);
    }
    free(il);
}

void il_clear(IntList* il) {
    il->num = 0;
    il->free_element = -1;
}

int il_size(IntList* il) {
    return il->num;
}

int il_get(IntList* il, int n, int field) {
    assert(n >= 0 && n < il->num);
    return il->data[n*il->num_fields + field];
}

void il_set(IntList* il, int n, int field, int val) {
    assert(n >= 0 && n < il->num);
    il->data[n*il->num_fields + field] = val;
}

int il_push_back(IntList* il) {
    const int new_pos = (il->num+1) * il->num_fields;

    // If the list is full, we need to reallocate the buffer to make room
    // for the new element.
    if (new_pos > il->cap) {
        // Use double the size for the new capacity.
        const int new_cap = new_pos * 2;

        // If we're pointing to the fixed buffer, allocate a new array on the
        // heap and copy the fixed buffer contents to it.
        if (il->cap == il_fixed_cap) {
            il->data = malloc(new_cap * sizeof(*il->data));
            memcpy(il->data, il->fixed, sizeof(il->fixed));
        } else {
            // Otherwise reallocate the heap buffer to the new size.
            il->data = realloc(il->data, new_cap * sizeof(*il->data));
        }
        // Set the old capacity to the new capacity.
        il->cap = new_cap;
    }
    return il->num++;
}

void il_pop_back(IntList* il) {
    // Just decrement the list size.
    assert(il->num > 0);
    --il->num;
}

int il_insert(IntList* il) {
    // If there's a free index in the free list, pop that and use it.
    if (il->free_element != -1) {
        const int index = il->free_element;
        const int pos = index * il->num_fields;

        // Set the free index to the next free index.
        il->free_element = il->data[pos];

        // Return the free index.
        return index;
    }
    // Otherwise insert to the back of the array.
    return il_push_back(il);
}

void il_erase(IntList* il, int n) {
    // Push the element to the free list.
    const int pos = n * il->num_fields;
    il->data[pos] = il->free_element;
    il->free_element = n;
}
