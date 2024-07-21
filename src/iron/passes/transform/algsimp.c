#include "iron/iron.h"

/* pass "algsimp" - algebraic simplification

    constant evaluation:
        1 + 2   -> 3
        1 - 2   -> -1
        2 * 2   -> 4
        6 / 2   -> 3
        (neg) 1 -> -1

    strength reduction:
        x * 2 -> x << 1 // extend to powers of two
        x / 2 -> x >> 1 // extend to powers of two
        x % 2 -> x & 1  // extend to powers of two (is this valid? please tell me)

        x * 0  -> 0
        x & 0  -> 0
        x | 0  -> x
        x / 1  -> x
        x << 0 -> x
        x >> 0 -> x

    reassociation:
        (x + 1) + 2  ->  x + (1 + 2)
        (x * 1) * 2  ->  x * (1 * 2)
        (x & 1) & 2  ->  x & (1 & 2)
        (x | 1) | 2  ->  x | (1 | 2)
*/

static bool both_sides_const(FeInst* inst) {
    if (!(inst->kind > _FE_BINOP_BEGIN && inst->kind < _FE_BINOP_END)) return false;
    FeInstBinop* binop = (FeInstBinop*) inst;
    return binop->lhs->kind == FE_INST_LOAD_CONST && binop->rhs->kind == FE_INST_LOAD_CONST;
}

static FeInst* constant_evaluation(FeInst* inst) {

    // (sandwich): im boutta do some CURSED shit
    static_assert(sizeof(FeInstBinop) >= sizeof(FeInstLoadConst));
    
    FeInstLoadConst* lc = (FeInstLoadConst*) inst;

    switch (inst->kind) {
    case FE_INST_ADD:
    }
    TODO("");
    return inst;
}

static bool is_const_one(FeInst* inst) {
    if (inst->kind != FE_INST_LOAD_CONST) return false;

    FeInstLoadConst* lc = (FeInstLoadConst*) inst;
    switch (lc->base.type->kind) {
    case FE_I64: return lc->i64 == (i64) 1;
    case FE_I32: return lc->i32 == (i32) 1;
    case FE_I16: return lc->i16 == (i16) 1;
    case FE_I8:  return lc->i8  == (i8 ) 1;
    case FE_F64: return lc->f64 == (f64) 1;
    case FE_F32: return lc->f32 == (f32) 1;
    case FE_F16: return lc->f16 == (f16) 1;
    default: break;
    }
    return false;
}

static bool is_const_zero(FeInst* inst) {
    if (inst->kind != FE_INST_LOAD_CONST) return false;

    FeInstLoadConst* lc = (FeInstLoadConst*) inst;
    switch (lc->base.type->kind) {
    case FE_I64: return lc->i64 == (i64) 0;
    case FE_I32: return lc->i32 == (i32) 0;
    case FE_I16: return lc->i16 == (i16) 0;
    case FE_I8:  return lc->i8  == (i8 ) 0;
    case FE_F64: return lc->f64 == (f64) 0;
    case FE_F32: return lc->f32 == (f32) 0;
    case FE_F16: return lc->f16 == (f16) 0;
    default: break;
    }
    return false;
}

static FeInst* strength_reduction(FeInst* inst) {
    switch (inst->kind) {
        break;
    default:
        break;
    }
    return inst;
}

void run_pass_algsimp(FeModule* mod) {

}