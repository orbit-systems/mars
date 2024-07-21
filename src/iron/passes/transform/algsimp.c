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

    identity reduction:
        x + 0      -> x
        x - 0      -> x
        0 - x      -> -x
        x * 0      -> 0
        x * 1      -> x
        x / 1      -> x
        x & 0      -> 0
        x | 0      -> x
        x ^ 0      -> x
        x << 0     -> x
        x >> 0     -> x
        x && false -> false
        x && true  -> x
        x || false -> x
        x || true  -> true
        !(!x)      -> x
        (-x)      -> x

    reassociation:
        (x + 1) + 2  ->  x + (1 + 2)
        (x * 1) * 2  ->  x * (1 * 2)
        (x & 1) & 2  ->  x & (1 & 2)
        (x | 1) | 2  ->  x | (1 | 2)
*/

static bool binop_both_sides_const(FeInst* inst) {
    if (!(inst->kind > _FE_BINOP_BEGIN && inst->kind < _FE_BINOP_END)) return false;
    FeInstBinop* binop = (FeInstBinop*) inst;
    return binop->lhs->kind == FE_INST_LOAD_CONST && binop->rhs->kind == FE_INST_LOAD_CONST;
}

static FeInst* constant_evaluation(FeInst* inst) {

    // (sandwich): im boutta do some CURSED shit
    static_assert(sizeof(FeInstBinop) >= sizeof(FeInstLoadConst));
    
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

static FeInst* identity_reduction(FeInst* inst) {

    FeInstBinop* binop = (FeInstBinop*) inst;
    FeInstUnop* unop = (FeInstUnop*) inst;

    switch (inst->kind) {
    case FE_INST_ADD: // x + 0  -> x
    case FE_INST_SUB: // x - 0  -> x
    case FE_INST_OR:  // x | 0  -> x
    case FE_INST_XOR: // x ^ 0  -> x
    case FE_INST_SHL: // x << 0 -> x
    case FE_INST_LSR: // x >> 0 -> x
    case FE_INST_ASR: // x >> 0 -> x
        if (is_const_zero(binop->rhs)) {
            return binop->lhs;
        }
        break;
    case FE_INST_UMUL:
    case FE_INST_IMUL:
        if (is_const_zero(binop->rhs)) {
            return binop->rhs; // x * 0 -> 0
        }
        if (is_const_one(binop->rhs)) {
            return binop->lhs; // x * 1 -> x
        }
        break;
    case FE_INST_NOT:
    case FE_INST_NEG:
        if (unop->source->kind == inst->kind) {
            // !(!x) -> x
            // -(-x) -> x
            return ((FeInstUnop*)unop->source)->source;
        }
    default: break;
    }
    return inst;
}

static FeInst* strength_reduction(FeInst* inst) {
    switch (inst->kind) {
    default:
        break;
    }
    return inst;
}

void run_pass_algsimp(FeModule* mod) {

}