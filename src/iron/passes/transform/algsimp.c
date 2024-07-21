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

// if possible, replace the contents of target with source.
static bool replace_with(FeInst* target, FeInst* source) {
    if (fe_inst_sizes[target->kind] < fe_inst_sizes[source->kind]) {
        // source node is too big, we cant stuff it in
        return false;
    }

    memcpy(target, source, fe_inst_sizes[source->kind]);
    return true;
}

#define int_operate(op, dest, src1, src2) \
    switch (dest->base.type->kind) {\
    case FE_I64:  dest->i64  = src1->i64  op src2->i64;  break;\
    case FE_I32:  dest->i32  = src1->i32  op src2->i32;  break;\
    case FE_I16:  dest->i16  = src1->i16  op src2->i16;  break;\
    case FE_I8:   dest->i64  = src1->i8   op src2->i8;   break;\
    case FE_BOOL: dest->bool = src1->bool op src2->bool; break;\
    }

#define uint_operate(op, dest, src1, src2) \
    switch (dest->base.type->kind) {\
    case FE_I64:  dest->i64  = (u64) src1->i64  op (u64) src2->i64;  break;\
    case FE_I32:  dest->i32  = (u32) src1->i32  op (u32) src2->i32;  break;\
    case FE_I16:  dest->i16  = (u16) src1->i16  op (u16) src2->i16;  break;\
    case FE_I8:   dest->i64  = (u8)  src1->i8   op (u8)  src2->i8;   break;\
    case FE_BOOL: dest->bool =       src1->bool op       src2->bool; break;\
    }

static FeInst* const_eval_binop(FeFunction* f, FeInst* inst) {
    if (!(_FE_BINOP_BEGIN < inst->kind && inst->kind < _FE_BINOP_END)) {
        return inst;
    }
    FeInstBinop* binop = (FeInstBinop*) inst;

    if (binop->lhs->kind != FE_INST_LOAD_CONST || binop->rhs->kind != FE_INST_LOAD_CONST) {
        return inst;
    }

    FeInstLoadConst* src1 = (FeInstLoadConst*) binop->lhs;
    FeInstLoadConst* src2 = (FeInstLoadConst*) binop->rhs;
    FeInstLoadConst* lc   = (FeInstLoadConst*) fe_inst(f, FE_INST_LOAD_CONST);
    lc->base.type = inst->type;

    switch (inst->kind) {
    case FE_INST_ADD:   int_operate(+, lc, src1, src2); break;
    case FE_INST_SUB:   int_operate(-, lc, src1, src2); break;
    case FE_INST_IMUL:  int_operate(*, lc, src1, src2); break;
    case FE_INST_UMUL: uint_operate(*, lc, src1, src2); break;
    case FE_INST_IDIV:  int_operate(/, lc, src1, src2); break;
    case FE_INST_UDIV: uint_operate(/, lc, src1, src2); break;
    case FE_INST_AND:  uint_operate(&, lc, src1, src2); break;
    case FE_INST_OR:   uint_operate(|, lc, src1, src2); break;
    case FE_INST_XOR:  uint_operate(^, lc, src1, src2); break;
    case FE_INST_SHL:  uint_operate(<<, lc, src1, src2); break;
    case FE_INST_ASR:   int_operate(>>, lc, src1, src2); break;
    case FE_INST_LSR:  uint_operate(>>, lc, src1, src2); break;
    default:
        break;
    }
    
    return lc;
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
        break;
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