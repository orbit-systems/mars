#include "orbit.h"
#include "arena.h"
#include "error.h"

dynarr_lib(arena)

arena arena_make(size_t size) {
    arena a;
    a.raw = malloc(size);
    if (a.raw == NULL) {
        general_error("internal: arena size %lu too big, cannot allocate", size);
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
    u32 offset = a->offset;
    u32 new_offset = align_forward(a->offset, align) + size;
    if (new_offset > a->size) {
        return NULL;
    }
    a->offset = new_offset;
    return (void*)((uintptr_t)a->raw + align_forward(offset, align));
}

arena_list arena_list_make(size_t arena_size) {
    arena_list al;
    dynarr_init_arena(&al.list, 1);
    al.arena_size = arena_size;
    
    arena initial_arena = arena_make(al.arena_size);
    dynarr_append_arena(&al.list, initial_arena);

    return al;
}

void arena_list_delete(arena_list* restrict al) {
    FOR_URANGE_EXCL(i, 0, (al->list.len)) {
        arena_delete(&al->list.base[i]);
    }
    dynarr_destroy_arena(&al->list);
    *al = (arena_list){0};
}

void* arena_list_alloc(arena_list* restrict al, size_t size, size_t align) {
    // attempt to allocate at the top arena;
    void* attempt = arena_alloc(&al->list.base[al->list.len-1], size, align);
    if (attempt != NULL) return attempt; // yay!

    // FUCK! we need to append another arena block
    arena new_arena = arena_make(al->arena_size);
    dynarr_append_arena(&al->list, new_arena);

    // we're gonna try again
    attempt = arena_alloc(&al->list.base[al->list.len-1], size, align);
    return attempt; // this should ideally never be null
}


size_t align_forward(size_t ptr, size_t align) {
    if (!is_pow_2(align)) {
        general_error("internal: align is not a power of two (got %zu)\n", align);
    }
    
    size_t p = ptr;
    size_t mod = p & (align - 1);
    if (mod != 0) {
        p += align - mod;
    }
    return p;
}