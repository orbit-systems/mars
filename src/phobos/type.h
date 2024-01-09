#pragma once
#define PHOBOS_TYPE_H

#include "orbit.h"
#include "lexer.h"
#include "arena.h"
#include "dynarr.h"

typedef u8 mars_basic_type; enum {
    mtype_basic_invalid,

    mtype_basic_none,
    
    mtype_basic_u8,
    mtype_basic_u16,
    mtype_basic_u32,
    mtype_basic_u64,

    mtype_basic_i8,
    mtype_basic_i16,
    mtype_basic_i32,
    mtype_basic_i64,

    mtype_basic_f16,
    mtype_basic_f32,
    mtype_basic_f64,

    mtype_basic_bool,

    mtype_basic_addr,
};

#define TYPE_NODES \
    TYPE(basic, "basic", { \
        mars_basic_type kind; \
    }) \
/* type of expressions that return multiple types, like multi-return-value functions */ \
    TYPE(multi, "multi-type", { \
        dynarr(mars_type) subtypes; \
    }) \
    TYPE(struct, "struct", { \
        dynarr(mars_type) subtypes; \
        dynarr(size_t)    offsets; \
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

// generate typedefs
#define TYPE(ident, identstr, structdef) typedef struct mtype_##ident structdef mtype_##ident;
    TYPE_NODES
#undef TYPE

extern char* mt_type_str[];
extern size_t mt_type_size[];