#pragma once
#define DEIMOS_MIC_H

// MIL - mars intermediate language
// big shit real shit idk lmao

#include "orbit.h"
#include "arena.h"

typedef struct live_range {
    u32 definition;
    u32 last_use;
} live_range;

typedef struct mil_entity {
    string name;
    live_range range;
} mil_entity;

da_typedef(mil_entity);


typedef u8 mil_inst_type; enum {
    inst_local_load,
    inst_local_store,
    inst_load,
    inst_store,

    inst_add,
    inst_sub,
    inst_imul,
    inst_umul,
    inst_idiv,
    inst_udiv,
    inst_mod,
    inst_rem,

    inst_and,
    inst_or,
    inst_nor,
    inst_not,
    inst_xor,
    inst_shl,
    inst_lsr,
    inst_asr,
    inst_bit,
    inst_sext,
    inst_zext,

    // floating point

    inst_f_to,
    inst_f_from,
    inst_fadd,
    inst_fsub,
    inst_fneg,
    inst_fmul,
    inst_fdiv,
    inst_fsqrt,
    inst_fmin,
    inst_fmax,
    inst_fsat,
    inst_fconv,

    // intrinsics

    inst_memcopy,
    inst_memzero,

    // for custom assembly blocks

    inst_asm,

    // TODO
};

typedef struct mil_inst {
    mil_inst_type type;
} mil_inst;

// basic block
typedef struct mil_bb {
    da(mil_entity) entities;


} mil_bb;