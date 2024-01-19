#pragma once
#define DA_H

#include <stdlib.h>
#include <stdio.h>

// sandwich's shitty """polymorphic""" dynamic array lib V2
// lean and mean w/ new functions

#define da(type) da_##type

#define da_typedef(type) typedef struct da_##type { \
    type * at; \
    size_t len; \
    size_t cap; \
} da_##type

// the array pointer is called 'at' so that you can use it like 'dynarray.at[i]'

#define da_init(da_ptr, capacity) do { \
    size_t c = (capacity); \
    if ((capacity) <= 0) c = 1; \
    (da_ptr)->len = 0; \
    (da_ptr)->cap = c; \
    (da_ptr)->at = malloc(sizeof((da_ptr)->at[0]) * c); \
    if ((da_ptr)->at == NULL) { \
        printf("(%s:%d) da_init malloc failed for capacity %d", (__FILE__), (__LINE__), c); \
        exit(1); \
    } \
} while (0)

#define da_append(da_ptr, element) do { \
    if ((da_ptr)->len == (da_ptr)->cap) { \
        (da_ptr)->cap *= 2; \
        (da_ptr)->at = realloc((da_ptr)->at, sizeof((da_ptr)->at[0]) * (da_ptr)->cap); \
        if ((da_ptr)->at == NULL) { \
            printf("(%s:%d) da_append realloc failed for capacity %d", (__FILE__), (__LINE__), (da_ptr)->cap); \
            exit(1); \
        } \
    } \
    (da_ptr)->at[(da_ptr)->len++] = element; \
} while (0)

#define da_shrink(da_ptr) do { \
    if ((da_ptr)->len == (da_ptr)->cap) break; \
    (da_ptr)->cap = (da_ptr)->len; \
    (da_ptr)->at *= realloc(arr->at, sizeof((da_ptr)->at[0]) * (da_ptr)->cap); \
    if ((da_ptr)->at == NULL) { \
        printf("(%s:%d) da_init realloc failed for capacity %d", (__FILE__), (__LINE__), (da_ptr)->cap); \
        exit(1); \
    } \
} while (0)

#define da_reserve(da_ptr, num_slots) do { \
    (da_ptr)->cap += num_slots; \
    (da_ptr)->at *= realloc((da_ptr)->at, sizeof((da_ptr)->at[0]) * (da_ptr)->cap); \
    if ((da_ptr)->at == NULL) { \
        printf("(%s:%d) da_reserve realloc failed for capacity %d", (__FILE__), (__LINE__), (da_ptr)->cap); \
        exit(1); \
    } \
} while (0)

#define da_destroy(da_ptr) do { \
    if ((da_ptr)->at == NULL) break; \
    free((da_ptr)->at); \
} while (0)

#define da_insert_at(da_ptr, element, index) do { \
    da_append(da_ptr, element); \
    memmove(&(da_ptr)->at[index + 1], &(da_ptr)->at[index], sizeof((da_ptr)->at[0]) * ((da_ptr)->len-index-1)); \
    (da_ptr)->at[index] = element; \
} while (0)

#define da_remove_at(da_ptr, index) do { \
    memmove(&(da_ptr)->at[index], &(da_ptr)->at[index + 1], sizeof((da_ptr)->at[0]) * ((da_ptr)->len-index-1)); \
    (da_ptr)->len--; \
} while (0)

#define da_unordered_remove_at(da_ptr, index) do { \
    (da_ptr)->at[index] = (da_ptr)->at[(da_ptr)->len - 1]; \
    (da_ptr)->len--; \
} while (0)