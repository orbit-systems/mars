#include "common/orbit.h"
#include "common/arena.h"
#include "common/alloc.h"

_ArenaBlock arena_block_make(size_t size);
void arena_block_delete(_ArenaBlock* a);
void* arena_block_alloc(_ArenaBlock* a, size_t size, size_t align);

typedef struct _ArenaBlock {
    void* raw;
    u32 offset;
    u32 size;
} _ArenaBlock;

_ArenaBlock arena_block_make(size_t size) {
    _ArenaBlock block;
    block.raw = mars_alloc(size);
    if (block.raw == NULL) {
        CRASH("internal: arena block size %zu too big, can't allocate", size);
    }
    block.size = (u32)size;
    block.offset = 0;
    return block;
}

void arena_block_delete(_ArenaBlock* block) {
    mars_free(block->raw);
    *block = (_ArenaBlock){0};
}

void* arena_block_alloc(_ArenaBlock* block, size_t size, size_t align) {
    u32 offset = block->offset;
    u32 new_offset = align_forward(block->offset, align) + size;
    if (new_offset > block->size) {
        return NULL;
    }
    block->offset = new_offset;
    return (void*)((size_t)block->raw + align_forward(offset, align));
}

Arena arena_make(size_t block_size) {
    Arena al;
    da_init(&al.list, 1);
    al.arena_size = block_size;

    _ArenaBlock initial_arena = arena_block_make(al.arena_size);
    da_append(&al.list, initial_arena);

    return al;
}

void arena_delete(Arena* al) {
    for_urange(i, 0, (al->list.len)) {
        arena_block_delete(&al->list.at[i]);
    }
    da_destroy(&al->list);
    *al = (Arena){0};
}

void* arena_alloc(Arena* al, size_t size, size_t align) {
    // attempt to allocate at the top arena_block;
    void* attempt = arena_block_alloc(&al->list.at[al->list.len - 1], size, align);
    if (attempt != NULL) return attempt; // yay!

    // FUCK! we need to append another arena_block block
    _ArenaBlock new_arena = arena_block_make(al->arena_size);
    da_append(&al->list, new_arena);

    // we're gonna try again
    attempt = arena_block_alloc(&al->list.at[al->list.len - 1], size, align);
    return attempt; // this should ideally never be null
}

size_t align_forward(size_t ptr, size_t align) {
    if (!is_pow_2(align)) {
        CRASH("internal: align is not a power of two (got %zu)\n", align);
    }

    return (ptr + align - 1) & ~(align - 1);
}