#ifndef VEC_H
#define VEC_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct VecHeader {
    uint32_t len;
    uint32_t cap;
} VecHeader;

#define _assert_ptr_ptr(pp) (((void)**(pp)), (pp))

#define Vec(T) T*
#define vec_stride(vec) sizeof(*(vec))
#define vec_header(vec) ((VecHeader*)((char*)vec - sizeof(VecHeader)))
#define vec_len(vec) vec_header(vec)->len
#define vec_cap(vec) vec_header(vec)->cap
#define vec_clear(vecptr) vec_len(*_assert_ptr_ptr(vecptr)) = 0

#define vec_new(T, initial_cap) ((Vec(T)) _vec_new(sizeof(T), initial_cap))
#define vec_reserve(vecptr, slots) _vec_reserve((void**)_assert_ptr_ptr(vecptr), vec_stride(*vecptr), slots)
#define vec_append(vecptr, item) do { \
    _vec_reserve1((void**)_assert_ptr_ptr(vecptr), vec_stride(*vecptr)); \
    (*vecptr)[vec_len(*vecptr)++] = (item); \
} while (0)
#define vec_shrink(vecptr) _vec_shrink((void**)_assert_ptr_ptr(vecptr), vec_stride(*vecptr))
#define vec_destroy(vecptr) _vec_destroy((void**)_assert_ptr_ptr(vecptr))

Vec(void) _vec_new(size_t stride, size_t initial_cap);
void _vec_reserve(Vec(void)* v, size_t stride, size_t slots);
void _vec_reserve1(Vec(void)* v, size_t stride);
void _vec_shrink(Vec(void)* v, size_t stride);
void _vec_destroy(Vec(void)* v);

#endif // VEC_H
