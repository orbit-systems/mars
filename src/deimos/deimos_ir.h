#pragma once
#define DEIMOS_IR_H

#include "orbit.h"
#include "../phobos/lex.h"
#include "../arena.h"
#include "../term.h"
#include "../phobos/ast.h"

typedef struct {
} ir_base;

// define all the IR node macros
#define IR_NODES \
    IR_TYPE(function_label, "label", { \
        ir_base base; \
        string label_text; \
    }) \
    IR_TYPE(arg_mov, "argument mov", { \
        ir_base base; \
        string identifier_name; \
        int argument; \
    }) \
    IR_TYPE(binary_op, "binary op", { \
        ir_base base; \
        string lhs; \
        string rhs; \
        string out; \
        operator op; \
    }) \
    IR_TYPE(return_stmt, "return statement", { \
        ir_base base; \
        string identifier_name; \
    })\

// generate the enum tags for the IR tagged union
typedef u16 ir_type; enum {
    ir_type_invalid,
#define IR_TYPE(ident, identstr, structdef) ir_type_##ident,
    IR_NODES
#undef IR_TYPE
    ir_type_COUNT,
};

// generate tagged union IR type
typedef struct IR {
    union {
        void* rawptr;
        ir_base * base;
#define IR_TYPE(ident, identstr, structdef) struct ir_##ident * as_##ident;
        IR_NODES
#undef IR_TYPE
    };
    ir_type type;
} IR;

da_typedef(IR);

typedef enum {
    NO_OP,
    ADD,
    SUB,
    MUL,
    DIV,
    OP_COUNT,
} operator;

extern char* operator_strings[];

// generate DIR node typedefs
#define IR_TYPE(ident, identstr, structdef) typedef struct ir_##ident structdef ir_##ident;
    IR_NODES
#undef IR_TYPE

#define NULL_IR ((IR){0})
#define is_null_IR(node) ((node).type == 0 || (node).rawptr == NULL)

extern char* ir_type_str[];
extern size_t ir_type_size[];

IR new_ir_entry(arena* restrict alloca, ir_type type);

void print_ir(da(IR) ir_elems);