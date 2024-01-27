#pragma once
#define PHOBOS_TYPE_H

#include "orbit.h"
#include "lexer.h"
#include "arena.h"
#include "da.h"

#define TYPE_NODES \
    TYPE(meta_type,  "type expression", {char _;}) /* for type expressions */ \
    TYPE(meta_stmt,  "statement", {char _;}) /* for statements */ \
    \
    TYPE(basic_none, "none", {char _;}) /* idk this is probably useful somehow */ \
    TYPE(basic_u8,   "u8",   {char _;}) \
    TYPE(basic_u16,  "u16",  {char _;}) \
    TYPE(basic_u32,  "u32",  {char _;}) \
    TYPE(basic_u64,  "u64",  {char _;}) \
    TYPE(basic_i8,   "i8",   {char _;}) \
    TYPE(basic_i16,  "i16",  {char _;}) \
    TYPE(basic_i32,  "i32",  {char _;}) \
    TYPE(basic_i64,  "i64",  {char _;}) \
    TYPE(basic_f16,  "f16",  {char _;}) \
    TYPE(basic_f32,  "f32",  {char _;}) \
    TYPE(basic_f64,  "f64",  {char _;}) \
    TYPE(basic_bool, "bool", {char _;}) \
    TYPE(basic_addr, "addr", {char _;}) \
    TYPE(untyped_bool,   "untyped bool", {char _;}) \
    TYPE(untyped_int,    "untyped int", {char _;}) \
    TYPE(untyped_char,   "untyped char", {char _;}) \
    TYPE(untyped_float,  "untyped float", {char _;}) \
    TYPE(untyped_pointer,"untyped pointer", {char _;}) \
    TYPE(untyped_string, "untyped string", {char _;}) \
    \
    TYPE(multi, "multi", { /* type of expressions that have multiple types, like multi-valued return functions */ \
        da(mars_type) subtypes; \
    }) \
    TYPE(struct, "struct", { \
        da(mars_struct_field) fields;\
    }) \
    TYPE(union, "union", { \
        da(mars_struct_field) fields;\
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
        string name; \
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

da_typedef(mars_type);
da_typedef(size_t);
da_typedef(string);
da_typedef(mars_struct_field);

// generate typedefs
#define TYPE(ident, identstr, structdef) typedef struct mtype_##ident structdef mtype_##ident;
    TYPE_NODES
#undef TYPE

extern char* mt_kind_str[];
extern size_t mt_kind_size[];

mars_type new_type_node(arena* restrict a, mt_kind type);
size_t size_of_type(mars_type t);
size_t align_of_type(mars_type t);
bool type_eq(mars_type a, mars_type b);

void init_type_graph(arena* restrict type_arena);