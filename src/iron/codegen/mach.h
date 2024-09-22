#pragma once

#include "iron/iron.h"

typedef struct FeMachBuffer       FeMachBuffer;
typedef struct FeMachVReg         FeMachVReg;
typedef struct FeMachImmediate    FeMachImmediate;
typedef struct FeMachBase         FeMachBase;
typedef struct FeMachInst         FeMachInst;
typedef struct FeMachInstTemplate FeMachInstTemplate;

typedef u32 FeMachVregList;
typedef u32 FeMachImmediateList;
typedef u32 FeMachInstTemplateRef;

da_typedef(FeMachVReg);
da_typedef(u32);
da_typedef(FeMachImmediate);

struct FeMachBuffer {
    FeModule* module;

    struct {

    } yuh;

    da(u32) vreg_refs;
    da(FeMachVReg) vregs;
    da(FeMachImmediate) immediates;
};

#define FE_MACH_VREG_UNASSIGNED (UINT16_MAX)

struct FeMachVReg {
    u16 class;
    u16 real;
};

enum {
    FE_MACH_SECTION = 1,
    
    FE_MACH_REGALLOC_BEGIN,
    FE_MACH_REGALLOC_END,

    FE_MACH_LOCAL_LABEL,
    FE_MACH_SYMBOL_LABEL,

    FE_MACH_DIRECTIVE_SYMBOLDEF,
    FE_MACH_DIRECTIVE_ALIGN,

    FE_MACH_DATA_ZERO,
    FE_MACH_DATA_FILL,
    FE_MACH_DATA_D8,
    FE_MACH_DATA_D16,
    FE_MACH_DATA_D32,
    FE_MACH_DATA_D64,
    FE_MACH_DATA_BYTESTREAM,

    FE_MACH_INST,
};

struct FeMachBase {
    u8 tag;
};

// machine instruction
struct FeMachInst {
    FeMachBase base;

    FeMachInstTemplateRef template; // instruction template index
    FeMachVregList uses;
    FeMachVregList defs;
    FeMachImmediateList imms;
};

enum {
    FE_MACH_IMM_CONST = 1,
    FE_MACH_IMM_SYMBOL,
};

struct FeMachImmediate {

};

// template for a machine instruction.
struct FeMachInstTemplate {

};