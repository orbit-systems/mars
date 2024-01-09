#pragma once
#define ARENA_H

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

dynarr_lib_h(arena)

typedef struct {
    dynarr(arena) list;
    u32 arena_size;
} arena_list;

arena_list arena_list_make(size_t size);
void  arena_list_delete(arena_list* restrict al);
void* arena_list_alloc(arena_list* restrict al, size_t size, size_t align);

size_t align_forward(size_t ptr, size_t align);