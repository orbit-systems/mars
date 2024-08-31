#pragma once

#include "iron/iron.h"

typedef struct FeMachBuffer FeMachBuffer; 
typedef struct FeMachRegister FeMachRegister;
typedef struct FeMachVReg FeMachVreg;
typedef struct FeMachItem FeMachItem;

struct FeMachBuffer {
    FeModule* module;

};

struct FeMachRegister {

};

#define FE_VREG_UNASSIGNED (UINT16_MAX)

typedef struct FeMachVReg {
    u16 class;
    u16 real;
} FeMachVReg;

typedef struct FeMachItem {

} FeMachItem;

// machine instruction.
typedef struct FeMachInst {
    u32 template; // instruction template
    FeMachVReg** uses;
    FeMachVReg** defs;
} FeMachInst;