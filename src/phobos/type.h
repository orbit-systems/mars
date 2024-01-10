#pragma once
#define PHOBOS_TYPE_H

#include "orbit.h"
#include "lexer.h"
#include "arena.h"
#include "dynarr.h"

#define TYPE_NODES \
    TYPE(basic_none, "none", {}) \
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
    \
/* type of expressions that with multiple types, like multi-valued return functions */ \
    TYPE(multi, "multi", { \
        dynarr(mars_type) subtypes; \
    }) \
    TYPE(struct, "struct", { \
        dynarr(mars_struct_field) fields;\
    }) \
    TYPE(union, "union", { \
        dynarr(mars_struct_field) fields;\
    }) \
    TYPE(pointer, "pointer", { \
        mars_type subtype; \
    }) \
    TYPE(slice, "slice", { \
        mars_type subtype; \
    }) \
    TYPE(array, "array", { \
        mars_type subtype; \
        size_t length; \
    }) \
    TYPE(alias, "alias", { \
        mars_type subtype; \
    }) \
    

// generate the enum tags for the tagged union
typedef u8 mt_kind; enum {
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
    mt_kind type;
} mars_type;

typedef struct {
    string ident;
    size_t offset;
    mars_type type;
} mars_struct_field;

dynarr_lib_h(mars_type)
dynarr_lib_h(size_t)
dynarr_lib_h(string)
dynarr_lib_h(mars_struct_field)

// generate typedefs
#define TYPE(ident, identstr, structdef) typedef struct mtype_##ident structdef mtype_##ident;
    TYPE_NODES
#undef TYPE

extern char* mt_kind_str[];
extern size_t mt_kind_size[];

mars_type new_type_node(arena_list* restrict al, mt_kind type);
size_t size_of_type(mars_type t);
size_t align_of_type(mars_type t);