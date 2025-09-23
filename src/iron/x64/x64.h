#ifndef FE_X64_H
#define FE_X64_H

#include "iron/iron.h"

enum : u8 {
    X64_GPR_RAX = 0b0000,
    X64_GPR_RBX = 0b0001,
    X64_GPR_RCX = 0b0010,
    X64_GPR_RDX = 0b0011,
    X64_GPR_RSP = 0b0100,
    X64_GPR_RBP = 0b0101,
    X64_GPR_RSI = 0b0110,
    X64_GPR_RDI = 0b0111,

    X64_GPR_R8  = 0b1000,
    X64_GPR_R9  = 0b1001,
    X64_GPR_R10 = 0b1010,
    X64_GPR_R11 = 0b1011,
    X64_GPR_R12 = 0b1100,
    X64_GPR_R13 = 0b1101,
    X64_GPR_R14 = 0b1110,
    X64_GPR_R15 = 0b1111,
};

typedef enum : FeInstKind {
    X64_ADD,
    X64_MOV,
} X64InstKind;

/*
    this becomes

    gpr
    [base]
    [base + i8]
    [base + i32]
    [base + index * scale]
    [base + index * scale + i8]
    [base + index * scale + i32]
*/

typedef enum : u8 {
    X64_OPM_REG,
    X64_OPM_BASE,
    X64_OPM_BASE_INDEX,
} X64OperandMode;

typedef struct {
    X64OperandMode op_mode;
    u8 scale;
    i32 disp;
} X64Inst;

typedef struct {
    X64OperandMode op_mode;
    u8 scale;
    i32 disp;

    i64 imm;
} X64InstImm;

#endif
