#pragma once

#include "iron/iron.h"

typedef struct FeMachBuffer       FeMachBuffer;
typedef struct FeMachVReg         FeMachVReg;
typedef struct FeMachImmediate    FeMachImmediate;
typedef struct FeMachSymbol       FeMachSymbol;

typedef struct FeMach             FeMach;
typedef struct FeMachInst         FeMachInst;
typedef struct FeMachInstTemplate FeMachInstTemplate;

typedef u32 FeMachVregList;
typedef u32 FeMachImmediateList; // index to first immediate

da_typedef(FeMachVReg);
da_typedef(u32);
da_typedef(FeMachImmediate);

typedef struct FeMachBuffer {
    
    struct {
        u16 arch;
        u16 system;
        void* arch_config;
        void* system_config;
        const FeMachInstTemplate* inst_templates;
    } target;

    struct {
        FeMach** at;
        u64 len;
        u64 cap;
    } buf;

    struct {
        FeMachSymbol* at;
        u64 len;
        u64 cap;
    } symtab;

    da(FeMachVReg) vregs;
    da(u32) vreg_lists;
    da(FeMachImmediate) immediates;

    Arena buf_alloca;
} FeMachBuffer;

#define FE_MACH_VREG_UNASSIGNED (UINT16_MAX)

typedef struct FeMachVReg {
    u8 class;
    u8 real;
    u8 hint;
} FeMachVReg;

enum {
    // JUST NOTHING LOL
    FE_MACH_NONE = 0,

    // define a section
    FE_MACH_SECTION = 1,

    // artificially begin/end a vreg lifetime, as if an instruction had defined/used a value for it
    // useful for dictating saved registers across calls, making sure parameter and return registers
    // are correctly handled, anything to do with reservations and calling conventions really
    // FeMachLifetimePoint
    FE_MACH_LIFETIME_BEGIN,
    FE_MACH_LIFETIME_END,

    // usually at the start of a function, tells the register allocator to start working
    FE_MACH_CFG_BEGIN,
    // usually at the end of a function, tells the register allocator to stop
    FE_MACH_CFG_END,

    // the following instruction WILL transfer control flow somewhere else.
    FE_MACH_CFG_JUMP,
    // the following instruction MIGHT transfer control flow somewhere else
    FE_MACH_CFG_BRANCH,
    // the following instruction is a target for a CFG jump/branch
    FE_MACH_CFG_TARGET,
    // the following instruction terminates a CFG branch
    FE_MACH_CFG_END,

    FE_MACH_LABEL_LOCAL,
    FE_MACH_LABEL_GLOBAL,

    FE_MACH_DATA_ZERO,
    FE_MACH_DATA_FILL,
    FE_MACH_DATA_D8,
    FE_MACH_DATA_D16,
    FE_MACH_DATA_D32,
    FE_MACH_DATA_D64,
    FE_MACH_DATA_BYTESTREAM,

    FE_MACH_INST,

    _FE_MACH_MAX,
};

typedef struct FeMach {
    u8 kind;
} FeMach;

typedef struct FeMachGlobalLabel {
    FeMach base;

    u32 symbol_index;
} FeMachGlobalLabel;

typedef struct FeMachLocalLabel {
    FeMach base;

    string name;
} FeMachLocalLabel;

typedef struct FeMachSection {
    FeMach base;

    string name;
} FeMachSection;

typedef struct FeMachLifetimePoint {
    FeMach base;

    u32 vreg;
} FeMachLifetimePoint;

// machine instruction
typedef struct FeMachInst {
    FeMach base;

    u16 template; // instruction template index
    FeMachVregList regs;
    FeMachImmediateList imms;
} FeMachInst;

enum {
    FE_MACH_IMM_CONST = 1,
    // FE_MACH_IMM_SYMBOL,
};

typedef struct FeMachImmediate {
    u8 kind;
    union {
        u64 d64;
        u32 d32;
        u16 d16;
        u8  d8;
    };
} FeMachImmediate;

// template for a machine instruction.
// machine instructions can have a maximum of 16 registers (counting def and use)
typedef struct FeMachInstTemplate {
    u16 template_index;
    u16 defs; // defs bitmask
    u16 uses; // uses bitmask
    u8 regs_len;
    u8 imms_len;
    bool side_effects : 1;
} FeMachInstTemplate;

typedef struct FeMachSymbol {
    char* name;
    u16 name_len;

    u8 binding;
    u32 def; // index of definition instruction
} FeMachSymbol;

FeMachBuffer fe_mach_codegen(FeModule* m);

FeType fe_mach_type_of_native_int(u16 arch);
FeType fe_mach_type_of_native_float(u16 arch);
bool   fe_mach_type_is_native(u16 arch, FeType t);

FeMach*     fe_mach_new(FeMachBuffer* buf, u8 kind);
FeMachInst* fe_mach_new_inst(FeMachBuffer* buf, u16 template_index);
FeMach*     fe_mach_new_lifetime_begin(FeMachBuffer* buf, u32 vreg);
FeMach*     fe_mach_new_lifetime_end(FeMachBuffer* buf, u32 vreg);

FeMach* fe_mach_append(FeMachBuffer* buf, FeMach* inst);

u32 fe_mach_new_vreg(FeMachBuffer* buf, u8 regclass);
u32 fe_mach_get_vreg(FeMachBuffer* buf, FeMachInst* inst, u8 index);
void fe_mach_set_vreg(FeMachBuffer* buf, FeMachInst* inst, u8 index, u32 vreg);

void fe_mach_regalloc(FeMachBuffer* buf);