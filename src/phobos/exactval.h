#pragma once
#define PHOBOS_EXACTVAL_H

#include "orbit.h"
#include "arena.h"

typedef struct exact_value exact_value;

typedef u8 exact_value_kind; enum {
    ev_invalid,
    
    ev_bool,
    ev_string,
    ev_untyped_int,
    ev_untyped_float,

    ev_u8,
    ev_u16,
    ev_u32,
    ev_u64,

    ev_i8,
    ev_i16,
    ev_i32,
    ev_i64,
    
    ev_f64,
    ev_f32,
    ev_f16,
    
    ev_pointer,
    ev_aggregate,
    ev_trustmebro, 
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