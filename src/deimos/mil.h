#pragma once
#define DEIMOS_MIC_H

// MIL - mars intermediate language

// it's a shitty IR, it does not lend itself to a lot of optimization
// but it works

#include "orbit.h"
#include "arena.h"

typedef struct live_range {
    u32 first;
    u32 last;
} live_range;

typedef u64 var_index;

typedef struct mil_variable {
    string name;
    live_range range;
} mil_variable;

da_typedef(mil_variable);

typedef struct mil_argument {
    union {
        i64 literal;
        var_index index;
    };
    bool is_literal;
} mil_argument;

/*
    inst_invalid,

    // "this instruction has been deleted"
    inst_deleted,

    // memory

    inst_local_load, // for local data
    inst_local_store, // for local data
    inst_load,
    inst_store,

    // branches

    inst_br_true
    inst_br_equal,
    inst_br_not_equal,
    inst_br_greater,
    inst_br_less,
    inst_br_greater_equal,
    inst_br_less_equal,
    inst_br_greater_unsigned,
    inst_br_less_unsigned,
    inst_br_greater_equal_unsigned,
    inst_br_less_equal_unsigned,

    inst_jump, // unconditional

    // procedure stuff

    inst_call,
    inst_return,

    inst_put_param,  // pass parameter
    inst_get_param,  // get a parameter value (inside a function)

    inst_put_return, // pass return value (inside a function)
    inst_get_return, // get a return value

    // standard ops

    inst_set, // for copy or load immeditates

    inst_add,
    inst_sub,
    inst_imul,
    inst_umul,
    inst_idiv,
    inst_udiv,
    inst_mod,
    inst_rem,

    inst_and,
    inst_or,
    inst_nor,
    inst_not,
    inst_xor,
    inst_shl,
    inst_lsr,
    inst_asr,
    inst_bit,  // extract bit
    inst_sext, // sign extend
    inst_zext, // zero extend

    // condition checking

    inst_check_equal,
    inst_check_not_equal
    inst_check_greater,
    inst_check_less,
    inst_check_greater_equal,
    inst_check_less_equal,
    inst_check_greater_unsigned,
    inst_check_less_unsigned,
    inst_check_greater_equal_unsigned,
    inst_check_less_equal_unsigned,

    inst_check_overflow,
    inst_check_underflow,

    // floating point

    inst_f_to,
    inst_f_from,
    inst_f_add,
    inst_f_sub,
    inst_f_neg,
    inst_f_mul,
    inst_f_div,
    inst_f_sqrt,
    inst_f_min,
    inst_f_max,
    inst_f_saturate,
    inst_f_convert,

    // intrinsics

    inst_memcopy,
    inst_memzero,
    inst_nop,

    // for custom assembly blocks

    inst_asm,
*/

#define INSTRUCTIONS \
    INSTR(local_load, { \
        var_index dest; \
        mil_argument offset; \
    })               \
    INSTR(local_store, { \
        var_index dest; \
        mil_argument offset; \
    })               \
    INSTR(load, { \
        var_index dest; \
        mil_argument src1; \
    })               \
    INSTR(store, { \
        mil_argument src1; \
        mil_argument src2; \
    }) \


typedef u8 mil_inst_type; enum {
    inst_invalid,
#define INSTR(ident, structdef) inst_##ident,
    INSTRUCTIONS
#undef INSTR
    inst_COUNT,
};

#define INSTR(ident, structdef) typedef struct mil_inst_##ident structdef mil_inst_##ident;
    INSTRUCTIONS
#undef INSTR

typedef struct mil_inst {
    mil_inst_type type;
    union {
#define INSTR(ident, structdef) mil_inst_##ident * as_##ident;
    INSTRUCTIONS
#undef INSTR
    };
} mil_inst;

da_typedef(mil_inst);

typedef struct mil_bb {
    string label;
    da(mil_inst) instrs;
} mil_bb;

typedef struct mil_function {
    struct mil_module * func;
    da(mil_variable) vars;
    size_t stack_size;
} mil_function;

typedef struct mil_static_region {
    
} mil_static_region;

da_typedef(mil_function);

typedef struct mil_module {
    arena str_alloca; // for allocating names and strings
    arena alloca;
    da(mil_function) functions;
} mil_module;