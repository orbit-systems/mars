#pragma once
#define PTRMAP_H

#include "orbit.h"
#include "alloc.h"

typedef struct PtrMap {
    void** keys;
    void** vals;
    size_t cap; // capacity
} PtrMap;

#define PTRMAP_NOT_FOUND ((void*)0xDEADBEEFDEADBEEF)

size_t hashfunc(void* str);

PtrMap* ptrmap_init(PtrMap* hm, size_t capacity);
void  ptrmap_reset(PtrMap* hm);
void  ptrmap_destroy(PtrMap* hm);
void  ptrmap_put(PtrMap* hm, void* key, void* val);
void  ptrmap_remove(PtrMap* hm, void* key);
void* ptrmap_get(PtrMap* hm, void* key);