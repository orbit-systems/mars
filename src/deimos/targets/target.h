#pragma once
#define DEIMOS_TARGET_H

#include "deimos.h"

#define REAL_REG_NOT_ASSIGNED ((u64)-1)

typedef struct TargetInstruction TargetInstruction;
typedef struct VirtualRegister VirtualRegister;
typedef struct MInst MInst;
typedef struct AsmSymbol AsmSymbol;

typedef struct VirtualRegister {
    // index of real register into regclass array (if REAL_REG_NOT_ASSIGNED, it has not been assigned yet)
    u16 real;

    // register class to pick from when assigning a real register
    // typically an integer GPR or a floating point GPR
    u16 required_regclass;
} VirtualRegister;

typedef struct ImmediateVal {
    union {
        i64 i64;
        f64 f64;
        f32 f32;
        f16 f16;
        AsmSymbol* sym;
    } as;
} ImmediateVal;

typedef struct Inst {

    // immediate values, length dictated by its TargetInstruction
    ImmediateVal* imms;

    // input virtual registers, length dictated by its TargetInstruction
    VirtualRegister** ins;

    // output virtual registers, length dictated by its TargetInstruction
    VirtualRegister** outs;

    // instruction kind information
    TargetInstruction* template;
} Inst;



/* TARGET DEFINITIONS AND INFORMATION */

// info about a register
typedef struct TargetRegisterInfo {
    // thing to print in the asm
    string name;

    // ignores writes and always reads zero
    bool tied_zero;
} TargetRegisterInfo;

typedef struct TargetRegisterClass {
    // register list
    TargetRegisterInfo* regs;
    u16 regs_len;

} TargetRegisterClass;

// codegen target definition
typedef struct TargetInfo {
    string name;

    TargetRegisterClass* regclasses;
    u16 regclasses_len;

    TargetInstruction* insts;
    u16 insts_len;

} TargetInfo;

// instruction template, each MInst follows one.
typedef struct TargetInstruction {
    string asm_string;

    u16 num_imms;
    u16 num_ins;
    u16 num_outs;

    // instruction specific stuffs

    bool is_mov; // is this a simple move?
} TargetInstruction;

/*
    format strings for assembly - example:
        addr {out 0}, {in 0}, {in 1}

    with ins = {rb, rc} and outs = {ra}, this 
    
    if one of the arguments is an immediate, use an immediate type instead of in or out.
        addr {out 0}, {in 0}, {int 0}

    immediate types supported are:
        sym     : prints a symbol name if applicable
        int     : prints as signed integer
        uint    : prints as unsigned integer
        hex     : prints in hexadecimal as unsigned integer
        bin     : prints in binary as unsigned integer
        f(bits) : prints as floating point number of (bits : {16, 32, 64}) width
*/
