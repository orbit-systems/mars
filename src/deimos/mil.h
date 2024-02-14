#pragma once
#define DEIMOS_ARES_H

#include "orbit.h"



typedef u8 mil_nodetype; enum {
    mil_invalid,

    mil_const,
    mil_localaddr,
    mil_symboladdr,

    mil_head,

    mil_load,
    mil_store,
    mil_vol_load,
    mil_vol_store,

    mil_cast,
    mil_bitcast,

    mil_add,
    mil_sub,
    mil_imul,
    mil_umul,
    mil_mod,
    mil_rem,

    mil_and,
    mil_or,
    mil_not,
    mil_xor,

    mil_fadd,
    mil_fsub,
    mil_fmul,
    mil_fdiv,
    mil_fneg,
    mil_fsqrt,

    mil_phi,

    mil_branch,
    mil_jump,
    mil_call,

    mil_asm,
    mil_memset,
    mil_memcopy,

};