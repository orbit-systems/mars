#pragma once
#define DEIMOS_TARGET_H

#include "deimos.h"

typedef struct TargetInstInfo      TargetInstInfo;
typedef struct TargetRegisterInfo  TargetRegisterInfo;
typedef struct TargetRegisterClass TargetRegisterClass;
typedef struct TargetFormatInfo    TargetFormatInfo;
typedef struct TargetInfo          TargetInfo;

typedef struct VReg VReg;
typedef struct AsmInst         AsmInst;
typedef struct AsmBlock        AsmBlock;
typedef struct AsmSymbol       AsmSymbol;
typedef struct AsmGlobal       AsmGlobal;
typedef struct AsmFunction     AsmFunction;
typedef struct AsmModule       AsmModule;

#define REAL_REG_UNASSIGNED (UINT32_MAX)

enum {
    VREG_NOT_SPECIAL,
    VREG_PARAMVAL, // extend this register's liveness to the beginning of the program
    VREG_RETURNVAL, // extend this register's liveness to the end of the program

    VREG_CALLPARAMVAL, // extend this register's liveness to the next call-classified instruction
    VREG_CALLRETURNVAL, // extend this register's liveness to the nearest previous call-classified instruction
};

typedef struct VReg {
    // index of real register into regclass array (if REAL_REG_UNASSIGNED, it has not been assigned yet)
    u32 real;

    // register class to pick from when assigning a machine register
    u32 required_regclass;

    // any special handling information
    u8 special;
} VReg;

enum {
    IMM_I64,
    IMM_U64,

    IMM_F64,
    IMM_F32,
    IMM_F16,
    IMM_SYM,
};

typedef struct ImmediateVal {
    union {
        i64 i64;
        u64 u64;

        f64 f64; // the float stuff isnt really useful i think
        f32 f32;
        f16 f16;
        AsmSymbol* sym;
    };
    u8 kind;
} ImmediateVal;

typedef struct AsmInst {

    // input virtual registers, length dictated by its TargetInstInfo
    VReg** ins;

    // output virtual registers, length dictated by its TargetInstInfo
    VReg** outs;

    // immediate values, length dictated by its TargetInstInfo
    ImmediateVal* imms;

    // instruction kind information
    TargetInstInfo* template;
} AsmInst;

typedef struct AsmBlock {
    AsmInst** at;
    u32 len;
    u32 cap;

    string label;

    AsmFunction* f;
} AsmBlock;

typedef struct AsmFunction {
    AsmBlock** blocks; // in order
    u32 num_blocks;

    struct {
        VReg** at;
        u32 len;
        u32 cap;
    } vregs;

    AsmSymbol* sym;
    AsmModule* mod;
} AsmFunction;

enum {
    SYMBIND_GLOBAL,
    SYMBIND_LOCAL,
    // more probably later
};

typedef struct AsmSymbol {
    string name;

    u8 binding;

    union {
        AsmGlobal* glob;
        AsmFunction* func;
    };

} AsmSymbol;

enum {
    GLOBAL_INVALID,
    GLOBAL_U8,
    GLOBAL_U16,
    GLOBAL_U32,
    GLOBAL_U64,

    GLOBAL_I8,
    GLOBAL_I16,
    GLOBAL_I32,
    GLOBAL_I64,

    GLOBAL_ZEROES,
    GLOBAL_BYTES,
};

typedef struct AsmGlobal {
    AsmSymbol* sym;

    union {

        u8  u8;
        u16 u16;
        u32 u32;
        u64 u64;
        
        i8  i8;
        i16 i16;
        i32 i32;
        i64 i64;

        struct {
            u8* data; // only used with GLOBAL_BYTES
            u32 len; // only used with GLOBAL_BYTES and GLOBAL_ZEROES
        };
    };

    u8 type;
} AsmGlobal;

typedef struct AsmModule {
    TargetInfo* target;

    AsmGlobal** globals;
    u32 globals_len;

    AsmFunction** functions;
    u32 functions_len;

    arena alloca;
} AsmModule;

/* TARGET DEFINITIONS AND INFORMATION */

// info about a register
typedef struct TargetRegisterInfo {
    // thing to print in the asm
    string name;
} TargetRegisterInfo;

typedef struct TargetRegisterClass {
    // register list
    TargetRegisterInfo* regs;
    u32 regs_len;

} TargetRegisterClass;

typedef struct TargetCallingConv {
    TargetRegisterInfo** param_regs;
    u16 param_regs_len;

    TargetRegisterInfo** return_regs;
    u16 return_regs_len;
} TargetCallingConv;

enum {
    ISPEC_NONE = 0, // no special handling
    ISPEC_MOVE, // register allocator should try to copy-elide this
    ISPEC_CALL, // register allocator needs to be careful about lifetimes over this
};

// instruction template, each MInst follows one.
typedef struct TargetInstInfo {
    string asm_string;

    u16 num_imms;
    u16 num_ins;
    u16 num_outs;

    // instruction specific stuffs

    u8 special; // any special information/classification?
} TargetInstInfo;

/*
    format strings for assembly - example:
        addr {out 0}, {in 0}, {in 1}

    with ins = {rb, rc} and outs = {ra}, this translates to:
        addr ra, rb, rc
    
    if some of the arguments are immediates, use 'imm'
        addr {out 0}, {in 0}, {imm 0}
*/

typedef struct TargetFormatInfo {
    string file_begin; // arbitrary text to put at the beginning
    string file_end; // arbitrary text to put at the end

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
    string block_label; // for things like basic block labels.

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
    u8 inst_align;

    TargetFormatInfo* format_info;

} TargetInfo;

AsmModule*   asm_new_module(TargetInfo* target);
AsmFunction* asm_new_function(AsmModule* m, AsmSymbol* sym);
AsmGlobal*   asm_new_global(AsmModule* m, AsmSymbol* sym);
AsmBlock*    asm_new_block(AsmFunction* f, string label);
AsmInst*     asm_add_inst(AsmBlock* b, AsmInst* inst);
AsmInst*     asm_new_inst(AsmModule* m, u32 template);

VReg*        asm_new_vreg(AsmModule* m, u32 regclass);