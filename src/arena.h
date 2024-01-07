#include "orbit.h"
#include "dynarr.h"

// memory arenas

typedef struct {
    void* raw;
    u32 offset;
    u32 size;
} arena;

arena arena_make(size_t size);
void  arena_delete(arena* restrict a);
void* arena_alloc(arena* restrict a, size_t size, size_t align);

dynarr_lib_h(arena);

typedef dynarr(arena) arena_list;

arena arena_list_make(size_t size);
void  arena_list_delete(arena_list* restrict a);
void* arena_list_alloc(arena_list* restrict a, size_t size, size_t align);

size_t align_forward(size_t ptr, size_t align);