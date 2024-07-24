#pragma once
#define ARENA_H

#include "common/orbit.h"

// memory arenas

typedef struct _ArenaBlock _ArenaBlock;

da_typedef(_ArenaBlock);

typedef struct Arena {
    da(_ArenaBlock) list;
    u32 arena_size;
} Arena;

Arena arena_make(size_t size);
void  arena_delete(Arena* al);
void* arena_alloc(Arena* al, size_t size, size_t align);

size_t align_forward(size_t ptr, size_t align);