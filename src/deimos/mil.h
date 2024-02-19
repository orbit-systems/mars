#pragma once
#define DEIMOS_MIL_H

#include "orbit.h"
#include "arena.h"

typedef u8 bitwidth; enum {
    width_1 = 0,
    width_2,
    width_4,
    width_8,
    width_16,
    width_32,
    width_64,
};

typedef u8 mil_typekind; enum {
    tk_void,
    tk_data,
    tk_memory,
    tk_control,
    tk_tuple,

    tk_phi, // phi connection; used by reserve and unify
};

typedef struct mil_conntype mil_conntype;

typedef struct mil_conntype {
    mil_typekind kind;
    u16 tuple_len; // if tuple, how big
    mil_typekind* tuple; // because tuples will never have tuples inside of them
} mil_conntype;

typedef u8 mil_nodekind; enum {
    mil_invalid,


    // function entry point
    // -> (control, memory, data...)
    mil_head,

    // block header
    // control... -> control
    mil_block,

    // control, data, data -> (control, control)
    mil_branch_eq,  // ins[1] == ins[2]
    mil_branch_ne,  // ins[1] != ins[2]
    mil_branch_lt,  // ins[1] <  ins[2]
    mil_branch_le,  // ins[1] <= ins[2]
    mil_branch_gt,  // ins[1] >  ins[2]
    mil_branch_ge,  // ins[1] >= ins[2]
    mil_branch_ltu, // ins[1] <  ins[2] unsigned
    mil_branch_leu, // ins[1] <= ins[2] unsigned
    mil_branch_gtu, // ins[1] >  ins[2] unsigned
    mil_branch_geu, // ins[1] >= ins[2] unsigned

    // control -> control
    mil_jump,

    // control, memory, data... -> (control, memory, data...)
    mil_call,

    // control, data... ->
    mil_return,

    // inline assembly block
    // kind of treated like a function call but with
    // control, memory, data... -> (control, memory, data...)
    mil_asm,


    // memory operations
    // control, memory, data -> data
    mil_load,
    // control, memory, data, data -> memory
    mil_store,

    // memory operations that cannot be optimized out
    // control, memory, data -> memory, data
    mil_vol_load,

    // control, memory, data, data -> memory
    mil_vol_store,

    // -> data
    mil_const,

    // -> data
    mil_localptr,

    // -> data
    mil_symbolptr,

    // tuple -> any
    mil_proj,

    // data, data -> data
    mil_add,
    mil_sub,
    mil_imul,
    mil_umul,
    mil_mod, // Python-like modulo
    mil_rem, // C-like modulo
    mil_and,
    mil_or,
    mil_xor,

    // data -> data
    mil_not,
    mil_zero_ext,
    mil_sign_ext,

    // extract bits (low-high)
    mil_iselect,
    mil_uselect,

    // outs for this are (result, did_over/underflow)
    // data, data -> (data, data)
    mil_add_overflow,
    mil_sub_underflow,

    // data, data -> data
    mil_check_eq,  // ins[1] == ins[2]
    mil_check_ne,  // ins[1] != ins[2]
    mil_check_lt,  // ins[1] <  ins[2]
    mil_check_le,  // ins[1] <= ins[2]
    mil_check_gt,  // ins[1] >  ins[2]
    mil_check_ge,  // ins[1] >= ins[2]
    mil_check_ltu, // ins[1] <  ins[2] unsigned
    mil_check_leu, // ins[1] <= ins[2] unsigned
    mil_check_gtu, // ins[1] >  ins[2] unsigned
    mil_check_geu, // ins[1] >= ins[2] unsigned

    // data -> data
    mil_to_float,
    mil_from_float,

    // data, data -> data
    mil_fadd,
    mil_fsub,
    mil_fmul,
    mil_fdiv,

    // data -> data
    mil_fneg,
    mil_fsqrt,

    // instead of phi nodes or block arguments, we do preserve and unify nodes
    // its basically phi nodes but easier to codegen imo
    // if you want to merge data from mutliple control flow paths,
    // use a mil_preserve node at the end of your basic block

    // control, data -> phi
    mil_preserve,

    // then use a unify node to take all of your phi connections from preserve nodes
    // and merge them into a single data connection
    // very nice to codegen, because mil_unify does not generate any real code
    // mil_preserve does most of the heavy lifting

    // phi... -> data
    mil_unify,

    mil_MAX,
};

static_assert(mil_MAX < (1 << (sizeof(mil_nodekind)*8)), "mil_nodekind exceeds 256");

/*

bb1:
    x1 = 0
    jump bb2
bb2:
    x2 = phi (bb2, x1), (bb3, x3)
    if x2 < 10 jump bb3 else bb4
bb3:
    x3 = x2 + 1
    jump bb2
bb4:
    return x2;




bb1:
    x1 = preserve(0)
    jump bb2
bb2:
    x2 = merge x1, x3
    if x2 < 10 jump bb3 else bb4
bb3:
    x3 = x2 + 1
    x4 = preserve(x3)
    jump bb2
bb4:
    return x2;


*/

typedef struct mil_module   mil_module;
typedef struct mil_function mil_function;
typedef struct mil_node     mil_node;
typedef struct mil_symbol   mil_symbol;

#define NODE_BASE       \
    mil_nodekind kind;  \
    u32 in_count;       \
    u32 use_count;      \
    mil_node** ins;     \
    mil_node** uses;


typedef struct mil_node {
    NODE_BASE
} mil_node;

typedef struct node_proj {
    NODE_BASE
    int index;
} node_proj;

typedef struct node_head {
    NODE_BASE
    mil_function* function;
} node_head;

typedef struct node_block {
    string label;
} node_block;


typedef struct mil_symbol {
    string label;
    bool external;
} mil_symbol;

typedef struct mil_function {
    mil_symbol* sym;
    mil_module* module;
    node_head* head;
    arena node_alloc;
} mil_function;

typedef struct mil_static {
    mil_symbol* sym;
    bitwidth align;
    size_t size;
} mil_static;

typedef struct mil_module {
    mil_function* functions;
    mil_static* storage;
    mil_symbol* symbol;
} mil_module;

