#ifndef IRON_SHORT_TRAITS_H
#define IRON_SHORT_TRAITS_H

#include "iron/iron.h"

enum : u16 {
    COMMU       = FE_TRAIT_COMMUTATIVE,
    // if something is associative, it is also fast-associative.
    ASSOC       = FE_TRAIT_ASSOCIATIVE,
    FAST_ASSOC  = FE_TRAIT_FAST_ASSOCIATIVE,

    VOL         = FE_TRAIT_VOLATILE,
    // term always implies volatile
    TERM        = FE_TRAIT_TERMINATOR,
    SAME_IN_OUT = FE_TRAIT_SAME_IN_OUT_TY,
    SAME_INS    = FE_TRAIT_SAME_INPUT_TYS,
    INT_IN      = FE_TRAIT_INT_INPUT_TYS,
    FLT_IN      = FE_TRAIT_FLT_INPUT_TYS,
    VEC_IN      = FE_TRAIT_VEC_INPUT_TYS,
    BOOL_OUT    = FE_TRAIT_BOOL_OUT_TY,
    
    MOV_HINT    = FE_TRAIT_REG_MOV_HINT,
    BINOP       = FE_TRAIT_BINOP,
    UNOP        = FE_TRAIT_UNOP,

    MEM_USE     = FE_TRAIT_MEM_USE,
    MEM_DEF     = FE_TRAIT_MEM_DEF,
};


#endif // IRON_SHORT_TRAITS_H
