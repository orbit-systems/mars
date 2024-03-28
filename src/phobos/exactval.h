#pragma once
#define PHOBOS_EXACTVAL_H

#include "orbit.h"
#include "arena.h"

typedef u8 exact_value_kind; enum {
    ev_invalid,
    ev_bool,
    ev_string,
    ev_int,
    ev_float,
    ev_pointer,
    ev_aggregate,
    // ev_trustme, /* the checker can't determine what the value is, but it WILL be constant. */
};

typedef struct exact_value {
    union {
        bool    as_bool;
        string  as_string;
        i64     as_int;
        f64     as_float;
        u64     as_pointer;
        struct {
            struct exact_value** vals; // REMEMBER TO ALLOCATE! its not a DA so this is manual
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