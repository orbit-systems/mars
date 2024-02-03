#pragma once
#define SHITTY_IR_H

#include "orbit.h"
#include "../phobos/phobos.h"

typedef u8 IR_operand_kind; enum {
    op_invalid,
    op_param, // parameter name
    op_literal, // integer literal
    op_entityptr, // entity pointer constant
};

typedef struct {
    union {
        i64 literal;
        string entityptr;
        string param_name;
    };
    IR_operand_kind kind;
} IR_operand;

typedef u8 IR_instr_kind; enum {
    ir_op,
    ir_load,
    ir_store,
};

typedef struct {
    u32 dest; // operand indexes
    u32 src1;
    u32 src2;
    IR_instr_kind kind;
} IR_instruction;

da_typedef(IR_instruction);
da_typedef(IR_operand);

typedef struct {
    da(IR_instruction) block;
    da(IR_operand) operands;
} IR_generator;