#pragma once
#define PHOBOS_EXACTVAL_H

#include "orbit.h"
#include "arena.h"

typedef struct exact_value exact_value;

typedef u8 exact_value_kind; enum {
    EV_INVALID,
    
    EV_BOOL,
    EV_STRING,
    EV_UNTYPED_INT,
    EV_UNTYPED_FLOAT,

    EV_U8,
    EV_U16,
    EV_U32,
    EV_U64,

    EV_I8,
    EV_I16,
    EV_I32,
    EV_I64,

    EV_F16,
    EV_F32,
    EV_F64,
    
    EV_POINTER,
    EV_AGGREGATE,
    // EV_TRUSTMEBRO, 
    /* the checker can't determine what the value is, 
       but it WILL be constant at compile time. this is
       usually due to pointer shenangians, where the value of the pointer
       will be known by the backend but isn't known by the frontend */
};

typedef struct exact_value {
    union {
        bool    as_bool;
        string  as_string;
        i64     as_untyped_int;
        f64     as_untyped_float;

        u8      as_u8;
        u16     as_u16;
        u32     as_u32;
        u64     as_u64;

        i8      as_i8;
        i16     as_i16;
        i32     as_i32;
        i64     as_i64;

        f64     as_f64;
        f32     as_f32;
        f16     as_f16;

        u64     as_pointer;

        struct {
            exact_value** vals; // REMEMBER TO ALLOCATE! its not a DA so this is manual
            u32 len;
        } as_aggregate;
    };
    exact_value_kind kind;
    bool freeable;
} exact_value;

#define NO_AGGREGATE (-1)
#define USE_MALLOC ((void*)1)

exact_value* new_exact_value(int aggregate_len, arena* restrict alloca);
void destroy_exact_value(exact_value* ev);