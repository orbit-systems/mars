#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/util.h"
#include "iron/iron.h"

static u8 extra_size_table[FE__INST_END] = {
    // default error value
    [0 ... FE__INST_END - 1] = 255,

    [FE__ROOT] = 0,
    [FE__BOOKEND] = sizeof(FeInst_Bookend),
    [FE_PROJ] = sizeof(FeInstProj),
    [FE_CONST] = sizeof(FeInstConst),
    [FE_SYM_ADDR] = sizeof(FeInstSymAddr),
    [FE_STACK_ADDR] = sizeof(FeInstStack),
    [FE_IADD ... FE_FREM] = 0,
    [FE_MOV ... FE_F2I] = 0,
    [FE_LOAD ... FE_MEM_BARRIER] = sizeof(FeInstMemop),
    [FE_UNREACHABLE] = 0,
    [FE_BRANCH] = sizeof(FeInstBranch),
    [FE_JUMP] = sizeof(FeInstJump),
    [FE_RETURN] = 0,
    [FE_PHI ... FE_MEM_PHI] = sizeof(FeInstPhi),
    [FE_CALL] = sizeof(FeInstCall),
    [FE__MACH_REG] = 0,
    [FE__MACH_RETURN] = 0,
    [FE__MACH_STACK_SPILL] = sizeof(FeInstStack),
    [FE__MACH_STACK_RELOAD] = sizeof(FeInstStack),
};

void fe__load_extra_size_table(usize start_index, const u8* table, usize len) {
    memcpy(&extra_size_table[start_index], table, sizeof(table[0]) * len);
}

usize fe_inst_extra_size_unsafe(FeInstKind kind) {
    u8 size = 255;
    if (kind < FE__INST_END) {
        size = extra_size_table[kind];
    }

    return (usize) size;
}

usize fe_inst_extra_size(FeInstKind kind) {
    u8 size = fe_inst_extra_size_unsafe(kind);
    if (size == 255) {
        FE_CRASH("invalid inst kind %u", kind);
    }
    return (usize) size;
}

#define IPOOL_CHUNK_DATA_SIZE 4096
struct Fe__InstPoolChunk {
    Fe__InstPoolChunk* next;
    usize used;
    usize data[IPOOL_CHUNK_DATA_SIZE];
    // set to non-zero so slot reclaimer doesn't
    // walk off the end of the chunk data on accident
    usize backstop;
};
struct Fe__InstPoolFreeSpace {
    Fe__InstPoolFreeSpace* next;
};

static Fe__InstPoolChunk* ipool_new_chunk() {
    Fe__InstPoolChunk* chunk = fe_malloc(sizeof(Fe__InstPoolChunk));
    chunk->next = nullptr;
    chunk->used = 0;
    chunk->backstop = 0xFF;
    return chunk;
}

void fe_ipool_init(FeInstPool* pool) {
    memset(pool, 0, sizeof(*pool));
    pool->top = ipool_new_chunk();
}

static FeInst* init_inst(FeInst* inst) {
    memset(inst, 0, sizeof(*inst));
    inst->kind = 0xAAAA;
    inst->vr_def = FE_VREG_NONE;
    return inst;
}

static void* ipool_alloc_raw(FeInstPool* pool, usize slots) {
    // try to allocate on the front block
    if (pool->top->used + slots <= IPOOL_CHUNK_DATA_SIZE) {
        // allocate on this block!
        FeInst* inst = (FeInst*)&pool->top->data[pool->top->used];
        pool->top->used += slots;
        return init_inst(inst);
    } 
    // we need to make a new block and allocate on this.
    Fe__InstPoolChunk* new_chunk = ipool_new_chunk();
    new_chunk->next = pool->top;
    pool->top = new_chunk;
    new_chunk->used = slots;
    return &new_chunk->data;;
}

FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size) {
    if (extra_size > FE__INST_EXTRA_MAX_SIZE)  {
        FE_CRASH("extra size > max size");
    }
    if (extra_size < sizeof(usize)) {
        extra_size = sizeof(usize);
    }

    usize extra_slots = extra_size / sizeof(usize);
    usize node_slots = extra_slots + sizeof(FeInst) / 8;

    // check if there's any reusable slots.
    // THE VOICESSSSSS
    for_n(i, extra_slots, FE__INST_EXTRA_MAX_SIZE / sizeof(usize) + 1) {
        if (pool->inst_free_spaces[i] != nullptr) {
            // pop from slot list
            FeInst* inst = (FeInst*)pool->inst_free_spaces[i];
            pool->inst_free_spaces[i] = pool->inst_free_spaces[i]->next;
            return init_inst(inst);
        }
    }

    FeInst* inst = (FeInst*) ipool_alloc_raw(pool, node_slots);
    return init_inst(inst);
}

void fe_ipool_free(FeInstPool* pool, FeInst* inst) {
    // set inst extra memory to zero
    usize* extra = fe_extra(inst);
    usize extra_size = fe_inst_extra_size(inst->kind);
    memset(extra, 0, extra_size);

    // reclaim slots
    usize size_class = extra_size / sizeof(pool->top->data[0]);
    // while (size_class < FE__IPOOL_INST_FREE_SPACES_LEN - 1 && extra[size_class] == 0) {
    //     size_class += 1;
    // }

    // add to free list
    Fe__InstPoolFreeSpace* free_space = (Fe__InstPoolFreeSpace*)inst;
    free_space->next = pool->inst_free_spaces[size_class];
    pool->inst_free_spaces[size_class] = free_space;
}

// "free" the memory without actually giving it back to the allocator
// return the amount of usable space available
usize fe_ipool_free_manual(FeInstPool* pool, FeInst* inst) {
    // set inst extra memory to zero
    usize* extra = fe_extra(inst);
    usize extra_size = fe_inst_extra_size(inst->kind);
    memset(extra, 0, extra_size);

    // reclaim slots
    usize size_class = extra_size / sizeof(pool->top->data[0]);
    while (size_class < FE__IPOOL_INST_FREE_SPACES_LEN - 1 && extra[size_class] == 0) {
        size_class += 1;
    }

    return size_class * sizeof(pool->top->data[0]) + sizeof(FeInst);
}

static inline usize usize_next_pow_2(usize x) {
    return 1 << ((sizeof(x) * 8) -_Generic(x,
        unsigned long long: __builtin_clzll(x - 1),
        unsigned long: __builtin_clzl(x - 1),
        unsigned int: __builtin_clz(x - 1)));
}

static inline usize usize_log2(usize x) {
    return (sizeof(x) * 8 - 1) - _Generic(x,
        unsigned long long: __builtin_clzll(x),
        unsigned long: __builtin_clzl(x),
        unsigned int: __builtin_clz(x));
}

#define lengthof(arr) (sizeof(arr) / sizeof(arr[0]))

// if not 3, must be a power of two
void* fe_ipool_list_alloc(FeInstPool* pool, usize list_cap) {
    // list_len = usize_next_pow_2(list_len);

    if_unlikely (!is_pow_2(list_cap)) {
        FE_CRASH("list_len is %zu, not a power of two", list_cap);
    }

    usize index = usize_log2(list_cap);

    if (index < lengthof(pool->lists_pow_2) && pool->lists_pow_2[index]) {
        Fe__InstPoolFreeSpace* space = pool->lists_pow_2[index];
        pool->lists_pow_2[index] = space->next;
        return (void**) space;
    }
    return ipool_alloc_raw(pool, list_cap);
}


void fe_ipool_list_free(FeInstPool* pool, void* list, usize list_cap) {
    // list_len = usize_next_pow_2(list_len);

    if_unlikely (!is_pow_2(list_cap)) {
        FE_CRASH("list_cap is %zu, not a power of two", list_cap);
    }

    usize index = usize_log2(list_cap);

    if (index < lengthof(pool->lists_pow_2)) {
        // insert into free list
        Fe__InstPoolFreeSpace* space = (Fe__InstPoolFreeSpace*) list;
        space->next = pool->lists_pow_2[index];
        pool->lists_pow_2[index] = space;
        return;
    }

    // list is too big! we must decompose into smaller chunks
    constexpr usize top_index = lengthof(pool->lists_pow_2) - 1;
    constexpr usize chunk_len = (1ull << top_index);

    while (list_cap != 0) {
        // insert into free list
        Fe__InstPoolFreeSpace* space = (Fe__InstPoolFreeSpace*) list;
        space->next = pool->lists_pow_2[top_index];
        pool->lists_pow_2[top_index] = space;

        // pointer arithmetic works out here i think???????
        list += chunk_len;
        list_cap -= chunk_len;
    }
}

void fe_ipool_destroy(FeInstPool* pool) {
    Fe__InstPoolChunk* top = pool->top;
    while (top != nullptr) {
        Fe__InstPoolChunk* this = top;
        top = top->next;
        fe_free(this);
    } 
    *pool = (FeInstPool){0};
}

#define ARENA_CHUNK_DATA_SIZE 32768
typedef struct Fe__ArenaChunk {
    Fe__ArenaChunk* prev;
    Fe__ArenaChunk* next;
    usize used;
    u8 data[ARENA_CHUNK_DATA_SIZE];
} Fe__ArenaChunk;

// assume align is a power of two
static inline uintptr_t align_forward(uintptr_t ptr, uintptr_t align) {
    return (ptr + align - 1) & ~(align - 1);
}

// return nullptr if cannot allocate
static void* chunk_alloc(Fe__ArenaChunk* ch, usize size, usize align) {
    usize new_used = align_forward(ch->used, align);
    if (new_used + size > ARENA_CHUNK_DATA_SIZE) {
        return nullptr;
    }
    ch->used = new_used;
    void* mem = &ch->data[ch->used];
    ch->used += size;
    return mem;
}

void fe_arena_init(FeArena* arena) {
    arena->top = fe_malloc(sizeof(*arena->top));
    arena->top->next = nullptr;
    arena->top->prev = nullptr;
    arena->top->used = 0;
}

void fe_arena_destroy(FeArena* arena) {
    Fe__ArenaChunk* top = arena->top;
    // traverse up to top of the list
    while (top->next) {
        top = top->next;
    }
    // destroy any saved blocks below
    for (Fe__ArenaChunk* ch = top, *prev = ch->prev; ch != nullptr; ch = prev, prev = prev->prev) {
        fe_free(ch);
    }
    arena->top = nullptr;
}

void* fe_arena_alloc(FeArena* arena, usize size, usize align) {
    void* mem = chunk_alloc(arena->top, size, align);
    if (mem) {
        return mem;
    }

    Fe__ArenaChunk* new_chunk = arena->top->next;
    if (new_chunk == NULL) {
        new_chunk = fe_malloc(sizeof(*new_chunk));
        new_chunk->prev = arena->top;
        new_chunk->next = nullptr;
        arena->top->next = new_chunk;
    }
    new_chunk->used = 0;
    arena->top = new_chunk;
    mem = chunk_alloc(new_chunk, size, align);
    if (mem) {
        return mem;
    }
    FE_CRASH("unable to arena-alloc size %zu align %zu", size, align);
}

FeArenaState fe_arena_save(FeArena* arena) {
    return (FeArenaState){
        .top = arena->top,
        .used = arena->top->used,
    };
}

void fe_arena_restore(FeArena* arena, FeArenaState save) {
    arena->top = save.top;
    arena->top->used = save.used;
}
