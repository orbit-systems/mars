#pragma once
#define DYNARR_H

#include <stdlib.h>

// sandwich's shitty """polymorphic""" dynamic array lib

#define dynarr(type) dynarr_##type

#define dynarr_init(type, arr, capacity) dynarr_init_##type(arr, capacity)
#define dynarr_append(type, arr, item) dynarr_append_##type(arr, item)
#define dynarr_shrink(type, arr) dynarr_shrink_##type(arr)
#define dynarr_destroy(type, arr) dynarr_destroy_##type(arr)

#define dynarr_lib_h(type)                                                      \
    typedef struct dynarr_##type##_s {                                          \
        type*  raw;                                                             \
        size_t len;                                                             \
        size_t cap;                                                             \
    } dynarr(type);                                                             \
    void dynarr_init_##type(dynarr(type)* arr, size_t capacity);                \
    void dynarr_append_##type(dynarr(type)* arr, type item);                    \
    void dynarr_shrink_##type(dynarr(type)* arr);                               \
    void dynarr_destroy_##type(dynarr(type)* arr);

#define dynarr_lib(type)                                                        \
    void dynarr_init_##type(dynarr(type)* arr, size_t capacity) {               \
        if (capacity == 0) capacity = 1;                                        \
        *arr = (dynarr(type)){NULL, 0, capacity};                               \
        arr->raw = (type*) malloc(sizeof(type) * capacity);                     \
        if (arr->raw == NULL) {                                                 \
            printf("failed to malloc (dynarr_init)\n");                         \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    }                                                                           \
    void dynarr_append_##type(dynarr(type)* arr, type item) {                   \
        if (arr == NULL) dynarr_init(type, arr, 1);                             \
        if (arr->len == arr->cap) {                                             \
            arr->cap *= 2;                                                      \
            arr->raw = (type*) realloc(arr->raw, sizeof(type) * arr->cap);      \
            if (arr->raw == NULL) {                                             \
                printf("FUCK the realloc failed (dynarr_append)\n");            \
                exit(EXIT_FAILURE);                                             \
            }                                                                   \
        }                                                                       \
        arr->raw[arr->len++] = item;                                            \
    }                                                                           \
    void dynarr_shrink_##type(dynarr(type)* arr) {                              \
        if (arr == NULL) {                                                      \
            dynarr_init(type, arr, 1);                                          \
            return;                                                             \
        }                                                                       \
        arr->raw = (type*) realloc(arr->raw, sizeof(type) * arr->len);          \
        if (arr->raw == NULL) {                                                 \
            printf("FUCK the realloc failed (dynarr_shrink)\n");                \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    }                                                                           \
    void dynarr_destroy_##type(dynarr(type)* arr) {                             \
        if (arr == NULL) {                                                      \
            return;                                                             \
        }                                                                       \
        free(arr->raw);                                                         \
    }
