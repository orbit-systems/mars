#include "alloc.h"

void* mars_alloc(size_t size) {
    void* p = malloc(size);
    if (!p) return NULL;

    memset(p, 0, size);
    return p;
}

void mars_free(void* ptr) {
    if (ptr) free(ptr);
}

void* mars_realloc(void* ptr, size_t new_size) {
    void* p = realloc(ptr, new_size);
    if (!p) return NULL;

    return p;
}