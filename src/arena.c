#include "arena.h"
#include "error.h"

arena arena_make(size_t size) {
    arena a;
    a.raw = malloc(size);
    if (a.raw == NULL) {
        general_error("internal error: arena size %lu too big, cannot allocate", size);
    }
    a.size = (u32) size;
    a.offset = 0;
    return a;
}

void arena_delete(arena* restrict a) {
    free(a->raw);
    *a = (arena){0};
}

void* arena_alloc(arena* restrict a, size_t size, size_t align) {
    u32 new_cursor = a->size;
}

size_t align_forward(size_t ptr, size_t align) {
    if (!is_pow_2(align)) {
        crash("align_backward align is not a power of two (got %zu)\n", align);
    }
    
    size_t p = ptr;
    size_t mod = p & (align - 1);
    if (mod != 0) {
        p += align - mod;
    }
    return p;
}