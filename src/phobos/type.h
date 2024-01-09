#pragma once
#define PHOBOS_TYPE_H

#include "orbit.h"
#include "lexer.h"
#include "arena.h"
#include "dynarr.h"


#define TYPE_NODES \
    TYPE(basic_none, "none", {}) /* like void */ \
    TYPE(basic_u8,   "u8",   {}) \
    TYPE(basic_u16,  "u16",  {}) \
    TYPE(basic_u32,  "u32",  {}) \
    TYPE(basic_u64,  "u64",  {}) \
    TYPE(basic_i8,   "i8",   {}) \
    TYPE(basic_i16,  "i16",  {}) \
    TYPE(basic_i32,  "i32",  {}) \
    TYPE(basic_i64,  "i64",  {}) \
    TYPE(basic_f16,  "f16",  {}) \
    TYPE(basic_f32,  "f32",  {}) \
    TYPE(basic_f64,  "f64",  {}) \
    TYPE(basic_bool, "bool", {}) \
    TYPE(basic_addr, "addr", {}) \
/* type of expressions that return multiple types, like multi-return-value functions */ \
    TYPE(multi, "multi-type", { \
        dynarr(mars_type) subtypes; \
    }) \
    TYPE(struct, "struct", { \
        dynarr(mars_type) subtypes; \
        dynarr(size_t)    offsets; \
        dynarr(string)    idents; \
    }) \
    TYPE(pointer, "pointer", { \
        mars_type underlying; \
    }) \
    

// generate the enum tags for the tagged union
typedef u16 mt_type; enum {
    mt_invalid,
#define TYPE(ident, identstr, structdef) mt_##ident,
    TYPE_NODES
#undef TYPE
    mt_COUNT
};

// generate tagged union AST type
typedef struct mars_type {
    union {
        void* rawptr;
#define TYPE(ident, identstr, structdef) struct mtype_##ident * as_##ident;
        TYPE_NODES
#undef TYPE
    };
    mt_type type;
} mars_type;

dynarr_lib_h(mars_type)
dynarr_lib_h(size_t)
dynarr_lib_h(string)

// generate typedefs
#define TYPE(ident, identstr, structdef) typedef struct mtype_##ident structdef mtype_##ident;
    TYPE_NODES
#undef TYPE

extern char* mt_type_str[];
extern size_t mt_type_size[];