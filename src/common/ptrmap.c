#include "common/ptrmap.h"

#define MAX_SEARCH 30

size_t hashfunc(void* key) {
    size_t hash = 5381;
    size_t k = (size_t)key;
    hash = ((hash << 5) + hash) + ((k >> 0) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 8) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 16) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 24) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 32) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 40) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 48) & 0xFF);
    hash = ((hash << 5) + hash) + ((k >> 56) & 0xFF);
    return hash;
}

void ptrmap_init(PtrMap* hm, size_t capacity) {
    hm->cap = capacity;
    hm->vals = mars_alloc(sizeof(hm->vals[0]) * hm->cap);
    hm->keys = mars_alloc(sizeof(hm->keys[0]) * hm->cap);
}

void ptrmap_destroy(PtrMap* hm) {
    if (hm->keys) mars_free(hm->keys);
    if (hm->vals) mars_free(hm->vals);
    *hm = (PtrMap){0};
}

void ptrmap_put(PtrMap* hm, void* key, void* val) {
    if (!key) return;
    size_t hash_index = hashfunc(key) % hm->cap;

    // mars_free slot
    if (hm->keys[hash_index] == NULL || hm->keys[hash_index] == key) {
        hm->keys[hash_index] = key;
        hm->vals[hash_index] = val;
        return;
    }

    // search for nearby mars_free slot
    for_urange(index, 1, min(MAX_SEARCH, hm->cap)) {
        size_t i = (index + hash_index) % hm->cap;
        if ((hm->keys[i] == NULL) || hm->keys[hash_index] == key) {
            hm->keys[i] = key;
            hm->vals[i] = val;
            return;
        }
    }

    // we gotta resize
    PtrMap new_hm;
    ptrmap_init(&new_hm, hm->cap * 2);

    // copy all the old entries into the new hasptrmap
    for (size_t i = 0; i < hm->cap; i++) {
        if (hm->keys[i] == NULL) continue;
        ptrmap_put(&new_hm, hm->keys[i], hm->vals[i]);
    }
    ptrmap_put(&new_hm, key, val);

    // destroy old map
    mars_free(hm->keys);
    mars_free(hm->vals);
    *hm = new_hm;
}

void* ptrmap_get(PtrMap* hm, void* key) {
    if (!key) return PTRMAP_NOT_FOUND;
    size_t hash_index = hashfunc(key) % hm->cap;

    // key found in first slot
    if (hm->keys[hash_index] == key) {
        return hm->vals[hash_index];
    }

    // linear search next slots
    for_urange(index, 1, min(MAX_SEARCH, hm->cap)) {
        size_t i = (index + hash_index) % hm->cap;
        if (hm->keys[i] == NULL) return PTRMAP_NOT_FOUND;
        if (hm->keys[i] == key) return hm->vals[i];
    }

    return PTRMAP_NOT_FOUND;
}

void ptrmap_remove(PtrMap* hm, void* key) {
    if (!key) return;

    size_t hash_index = hashfunc(key) % hm->cap;

    // key found in first slot
    if (hm->keys[hash_index] == key) {
        hm->keys[hash_index] = NULL;
        hm->vals[hash_index] = NULL;
        return;
    }

    // linear search next slots
    for_urange(index, 1, min(MAX_SEARCH, hm->cap)) {
        size_t i = (index + hash_index) % hm->cap;
        if (hm->keys[i] == NULL) return;
        if (hm->keys[hash_index] == key) {
            hm->keys[hash_index] = NULL;
            hm->vals[hash_index] = NULL;
            return;
        }
    }
}

void ptrmap_reset(PtrMap* hm) {
    memset(hm->vals, 0, sizeof(hm->vals[0]) * hm->cap);
    memset(hm->keys, 0, sizeof(hm->keys[0]) * hm->cap);
}