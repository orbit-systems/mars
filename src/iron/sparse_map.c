#include "iron/iron.h"

// more traditional hashmap
// better at handling sparsely distributed keys

void fe_smap_init(FeSparseMap* smap, u32 initial_cap) {
    smap->cap = initial_cap;
    smap->map = fe_zalloc(sizeof(smap->map[0]) * initial_cap);
    // smap->values = fe_malloc(sizeof(smap->values[0]) * initial_cap);
}

void fe_smap_destroy(FeSparseMap* smap) {
    fe_free(smap->map);
    *smap = (FeSparseMap){};
}

#define TOMBSTONE (1)

void fe_smap_put(FeSparseMap* smap, uintptr_t key, uintptr_t value) {
    uintptr_t index = key % smap->cap;

    if (((smap->size * 4) / smap->cap) > 3) {
        // resize if load factor is greater than 3/4
        FE_CRASH("todo resize sparse map");
    }

    while (true) {
        if (smap->map[index].key == 0 || smap->map[index].key == TOMBSTONE) {
            break;
        }
        if (smap->map[index].key == key) {
            smap->size += 1;
            break;
        }

        index += 1;
        if (index >= smap->cap) {
            index = 0;
        }
    }

    smap->map[index].key = key;
    smap->map[index].value = value;
}
