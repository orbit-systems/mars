#pragma once
#define PHOBOS_TYPE_H

#include "common/orbit.h"
#include "common/alloc.h"

enum MarsType {
    // does not really do anything. serves as an invalid/void type.
    TYPE_NONE,

    TYPE_UNTYPED_INT,
    TYPE_UNTYPED_FLOAT,

    // signed integer types
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    // unsigned integer types
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    // floating point types
    TYPE_F16,
    TYPE_F32,
    TYPE_F64,
    // boolean type
    TYPE_BOOL,

    // for checking purposes, is not an actual type lol
    TYPE_META_INTEGRAL,

    // reference to another type
    TYPE_POINTER,

    // reference to an array of another type + len
    TYPE_SLICE,

    // array of a type
    TYPE_ARRAY,

    // enumeration over an integral type
    TYPE_ENUM,

    // aggregate types
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_UNTYPED_AGGREGATE,

    // function type! has sort of the same semantics as a struct
    TYPE_FUNCTION,

    // an alias is an "entry point" for the type graph
    // an outside system can point to an alias and
    // the canonicalizer will make sure the alias never becomes invalid
    // references to an alias type get canonicalized into
    // references to the underlying type
    TYPE_ALIAS,
    // like an alias, except checks against other types always fail.
    // references do not get canonicalized away like aliases
    TYPE_DISTINCT,
};

typedef struct Type Type;
typedef Type* TypePTR;
da_typedef(TypePTR);

typedef struct TypeStructField {
    string name;
    Type* subtype;
    u64 offset;
} TypeStructField;

da_typedef(TypeStructField);

typedef struct TypeEnumVariant {
    string name;
    i64 enum_val;
} TypeEnumVariant;

da_typedef(TypeEnumVariant);

typedef struct Type {
    union {
        struct {
            string name; // only used by TYPE_ALIAS
            Type* subtype;
            bool mutable; // only used by pointers and slices
        } as_reference;   // used by TYPE_POINTER, TYPE_SLICE, TYPE_ALIAS, and TYPE_DISTINCT
        struct {
            Type* subtype;
            u64 len;
        } as_array;
        struct {
            da(TypeStructField) fields;
        } as_aggregate;
        struct {
            da(TypePTR) params;
            da(TypePTR) returns;
        } as_function;
        struct {
            da(TypeEnumVariant) variants;
            Type* backing_type;
        } as_enum;
    };
    Type* moved;
    u8 tag;
    bool visited : 1;
    u16 type_nums[2];

    u32 size; // NOTE: types dont have size OR alignment until you call the type_real_(size/align)_of(T) function
    u32 align;
} Type;

typedef struct {
    Type** at;
    size_t len;
    size_t cap;
} TypeGraph;

extern TypeGraph typegraph;

void make_type_graph();
Type* make_type(u8 tag);

void type_add_param(Type* s, Type* sub);
Type* type_get_param(Type* s, size_t i);
void type_add_return(Type* s, Type* sub);
Type* type_get_return(Type* s, size_t i);

void type_add_field(Type* s, string name, Type* sub);
TypeStructField* type_get_field(Type* s, size_t i);
void type_add_variant(Type* e, string name, i64 val);
TypeEnumVariant* type_get_variant(Type* e, size_t i);
void type_set_target(Type* p, Type* dest);
Type* type_get_target(Type* p);
u64 type_get_index(Type* t);

Type* type_get_from_num(u16 num, int num_set);
bool type_equivalent(Type* a, Type* b, bool* executed_TSA);
bool type_element_equivalent(Type* a, Type* b, int num_set_a, int num_set_b);
void type_locally_number(Type* t, u64* number, int num_set);
void type_reset_numbers(int num_set);
void type_canonicalize_graph();
void merge_type_references(Type* dest, Type* src, bool disable);

void print_type_graph();

// a < b
bool type_enum_variant_less(TypeEnumVariant* a, TypeEnumVariant* b);

u32 type_real_size_of(Type* t);
u32 type_real_align_of(Type* t);

bool type_is_infinite(Type* t);

Type* type_unalias(Type* t);