#pragma once
#define ARENA_H

#include "orbit.h"

// memory arenas

typedef struct {
    void* raw;
    u32 offset;
    u32 size;
} arena_block;

arena_block arena_block_make(size_t size);
void  arena_block_delete(arena_block* a);
void* arena_block_alloc(arena_block* a, size_t size, size_t align);

da_typedef(arena_block);

typedef struct arena {
    da(arena_block) list;
    u32 arena_size;
} arena;

arena arena_make(size_t size);
void  arena_delete(arena* al);
void* arena_alloc(arena* al, size_t size, size_t align);

size_t align_forward(size_t ptr, size_t align);