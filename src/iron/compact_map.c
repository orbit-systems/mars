#include "common/util.h"

#include "iron/iron.h"
#include <string.h>

static inline usize count_trailing_zeros(usize n) {
#if USIZE_MAX == UINT64_MAX
    return __builtin_ctzll(n);
#else
    return __builtin_ctz(n);
#endif
}

#define USIZE_BITS (sizeof(usize) * 8)

void fe_cmap_init(FeCompactMap* cmap) {
    *cmap = (FeCompactMap){};
    cmap->id_start = UINT32_MAX;
}

bool fe_cmap_contains_key(FeCompactMap* cmap, u32 key) {
    u32 id = key;
    u32 id_block = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    if_likely (cmap->id_start <= id_block && id_block < cmap->id_end) {
        usize exists_block = cmap->exists[id_block - cmap->id_start];
        return (exists_block & id_bit) != 0;
    }
    return false;
}

uintptr_t* fe_cmap_get(FeCompactMap* cmap, u32 key) {
    u32 id = key;
    u32 id_block = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    if_likely (cmap->id_start <= id_block && id_block < cmap->id_end) {
        usize exists_block = cmap->exists[id_block - cmap->id_start];
        if ((exists_block & id_bit) != 0) {
            return &cmap->values[id - cmap->id_start * USIZE_BITS];
        }
        return nullptr;
    }
    return nullptr;
}

void fe_cmap_put(FeCompactMap* cmap, u32 key, uintptr_t val) {
    u32 id = key;
    u32 id_block = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    // construct the initial state for the set centered around this inst
    if_unlikely (cmap->id_start == UINT32_MAX) {
        cmap->id_start = id_block;
        cmap->id_end   = id_block + 1;
        cmap->exists = fe_malloc(sizeof(usize));
        cmap->values = fe_malloc(sizeof(cmap->values[0]) * USIZE_BITS);

        cmap->exists[0] = id_bit; 
        cmap->values[id - cmap->id_start * USIZE_BITS] = val;

        return;
    }

    // printf("push %u %p\n", id, inst);

    if_likely (cmap->id_start <= id_block && id_block < cmap->id_end) {
        usize exists_block = cmap->exists[id_block - cmap->id_start];
        // printf("> %064lb\n", exists_block);
        if (!(exists_block & id_bit)) {
            exists_block |= id_bit;
            cmap->exists[id_block - cmap->id_start] = exists_block;
            cmap->values[id - cmap->id_start * USIZE_BITS] = val;
        }
        // printf("> %064lb\n\n", exists_block);
        return;
    }

    if (cmap->id_end <= id_block) {
        // expand upwards
        usize new_end = id_block + 1;
        usize new_size = new_end - cmap->id_start;
        // we can realloc
        cmap->values = fe_realloc(cmap->values, sizeof(FeInst*) * new_size * USIZE_BITS);
        cmap->exists = fe_realloc(cmap->exists, sizeof(FeInst*) * new_size);
        // have to memset the newly allocated '.exists' space
        for_n (i, cmap->id_end, new_end) {
            cmap->exists[i] = 0;
        }
        // do this more efficiently later idk
        cmap->id_end = new_end;
        cmap->exists[id_block] = id_bit;
        cmap->values[id - cmap->id_start * USIZE_BITS] = val;
    } else {
        // expand downwards
        FE_CRASH("fuck! implement downwards set expansion");
    }

}

void fe_cmap_remove(FeCompactMap* cmap, u32 key) {
    u32 id = key;
    u32 exists_block_index = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    if_likely (cmap->id_start <= exists_block_index && exists_block_index < cmap->id_end) {
        usize exists_block = cmap->exists[exists_block_index - cmap->id_start];
        if (exists_block & id_bit) {
            cmap->exists[exists_block_index - cmap->id_start] = exists_block ^ id_bit;
        }
    }
}

uintptr_t fe_cmap_pop_next(FeCompactMap* cmap) {

    u32 blocks_len = cmap->id_end - cmap->id_start;
    for_n (exists_block_index, 0, blocks_len) {

        usize exists_block = cmap->exists[exists_block_index];
        if (exists_block == 0) {
            // we can skip this block
            continue;
        }

        // pop the next instruction in the set
        usize set_bit = count_trailing_zeros(exists_block);
        // return the instruction at that position
        uintptr_t value = cmap->values[set_bit + exists_block_index * USIZE_BITS];

        // printf("pop %u %p\n", inst->id, inst);

        // remove it from its 'exists' block
        // printf("> %064lb\n", iset->exists[exists_block_index]);
        cmap->exists[exists_block_index] ^= (usize)(1) << set_bit;
        // printf("> %064lb\n\n", iset->exists[exists_block_index]);

        return value;
    }

    return 0;
}

void fe_cmap_destroy(FeCompactMap* cmap) {
    fe_free(cmap->values);
    fe_free(cmap->exists);
    *cmap = (FeCompactMap){0};
}
