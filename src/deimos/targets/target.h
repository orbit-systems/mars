#pragma once
#define DEIMOS_TARGET_H

#include "deimos.h"

#define REAL_REG_NOT_ASSIGNED ((u64)-1)

typedef struct TargetInstInfo TargetInstInfo;
typedef struct VirtualRegister VirtualRegister;
typedef struct AsmInst   AsmInst;
typedef struct AsmBlock  AsmBlock;
typedef struct AsmSymbol AsmSymbol;
typedef struct AsmGlobal AsmGlobal;

typedef struct VirtualRegister {
    AsmInst* def;
    // index of real register into regclass array (if REAL_REG_NOT_ASSIGNED, it has not been assigned yet)
    u32 real;

    // register class to pick from when assigning a real register
    // typically an integer GPR or a floating point GPR
    u32 required_regclass;
} VirtualRegister;

typedef struct ImmediateVal {
    union {
        i64 i64;
        u64 u64;

        f64 f64;
        f32 f32;
        f16 f16;
        AsmSymbol* sym;
    };
} ImmediateVal;

typedef struct AsmInst {

    // immediate values, length dictated by its TargetInstInfo
    ImmediateVal* imms;

    // input virtual registers, length dictated by its TargetInstInfo
    VirtualRegister** ins;

    // output virtual registers, length dictated by its TargetInstInfo
    VirtualRegister** outs;

    // instruction kind information
    TargetInstInfo* template;
} AsmInst;

typedef struct AsmBlock {
    AsmInst* instructions;
    u32 len;
    u32 cap;
} AsmBlock;

typedef struct AsmFunction {
    AsmBlock** blocks;
    u32 num_blocks;

} AsmFunction;

typedef struct AsmSymbol {
    string name;

} AsmSymbol;

typedef struct AsmGlobal {

} AsmGlobal;


/* TARGET DEFINITIONS AND INFORMATION */

// info about a register
typedef struct TargetRegisterInfo {
    // thing to print in the asm
    string name;

    // the register allocator should
    bool regalloc_ignore;
} TargetRegisterInfo;

typedef struct TargetRegisterClass {
    // register list
    TargetRegisterInfo* regs;
    u32 regs_len;

} TargetRegisterClass;

// instruction template, each MInst follows one.
typedef struct TargetInstInfo {
    string asm_string;

    u16 num_imms;
    u16 num_ins;
    u16 num_outs;

    // instruction specific stuffs

    bool is_mov; // is this a simple move?
} TargetInstInfo;

typedef struct TargetFormatInfo {
    string u64;
    string u32;
    string u16;
    string u8;

    string i64;
    string i32;
    string i16;
    string i8;
    
    string zero; // filling a section with zero

    string string; // if NULL_STR, just use a bunch of `u8`
    char* escape_chars; // example: "\n\t"
    char* escape_codes; // example: "nt"
    u32 escapes_len; // example: 2

    string align;

    string label;
    string local_label; // for things like basic block labels.

    string bind_symbol_global;
    string bind_symbol_local;

    string begin_code_section;
    string begin_data_section;
    string begin_rodata_section;
    string begin_bss_section;

} TargetFormatInfo;

// codegen target definition
typedef struct TargetInfo {
    string name;

    TargetRegisterClass* regclasses;
    u32 regclasses_len;

    TargetInstInfo* insts;
    u32 insts_len;

    TargetFormatInfo* format_info;

} TargetInfo;

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