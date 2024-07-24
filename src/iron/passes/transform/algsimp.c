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
        sandwichman, note for you: what the FUCK does this mean?
            x && false -> false         this should be false
            x && true  -> true          this should be true only if x != 0, otherwise false
            x || false -> x             this should be true if x != 0, otherwise false
            x || true  -> true          this should be true, always
            !(!x)      -> x             this should be true if x != 0, otherwise false. i assume here you meant ~(~x)?

        -(-x)      -> x

    reassociation:
        (x + 1) + 2  ->  x + (1 + 2)
        (x * 1) * 2  ->  x * (1 * 2)
        (x & 1) & 2  ->  x & (1 & 2)
        (x | 1) | 2  ->  x | (1 | 2)
        xor is also associative
*/

// if possible, replace the contents of target with source.
// static bool replace_with(FeInst* target, FeInst* source) {
//     if (fe_inst_sizes[target->kind] < fe_inst_sizes[source->kind]) {
//         // source node is too big, we cant stuff it in
//         return false;
//     }

//     memcpy(target, source, fe_inst_sizes[source->kind]);
//     return true;
// }

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

    if (binop->lhs->kind != FE_INST_CONST || binop->rhs->kind != FE_INST_CONST) {
        return inst;
    }

    FeInstConst* src1 = (FeInstConst*) binop->lhs;
    FeInstConst* src2 = (FeInstConst*) binop->rhs;
    FeInstConst* lc   = (FeInstConst*) fe_inst_const(f);
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
    
    return (FeInst*) lc;
}


static FeInst* const_eval(FeFunction* f, FeInst* inst) {
    return const_eval_binop(f, inst);
}

static bool is_const_one(FeInst* inst) {
    if (inst->kind != FE_INST_CONST) return false;

    FeInstConst* lc = (FeInstConst*) inst;
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
    if (inst->kind != FE_INST_CONST) return false;

    FeInstConst* lc = (FeInstConst*) inst;
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

static FeInst* identity_reduction(FeInst* inst, bool* needs_inserting) {

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

bool is_const_power_of_two(FeInst* inst) {
    FeInstConst* lc = (FeInstConst*) inst;
    switch (lc->base.type->kind) {
    case FE_I64: return lc->i64 != 0 && (lc->i64 & (lc->i64 - 1)) == 0;
    case FE_I32: return lc->i32 != 0 && (lc->i32 & (lc->i32 - 1)) == 0;
    case FE_I16: return lc->i16 != 0 && (lc->i16 & (lc->i16 - 1)) == 0;
    case FE_I8:  return lc->i8  != 0 && (lc->i8  & (lc->i8  - 1)) == 0;
    default: break;
    }
    return false;
}

void convert_to_log2(FeInst* inst) {
    FeInstConst* lc = (FeInstConst*) inst;
    switch (lc->base.type->kind) {
    case FE_I64:lc->i64 = 8*sizeof(lc->i64) - __builtin_clzll(lc->i64) - 1; break;
    case FE_I32:lc->i32 = 8*sizeof(lc->i64) - __builtin_clzll(lc->i32) - 1; break;
    case FE_I16:lc->i16 = 8*sizeof(lc->i64) - __builtin_clzll(lc->i16) - 1; break;
    case FE_I8: lc->i8  = 8*sizeof(lc->i64) - __builtin_clzll(lc->i8 ) - 1; break;
    default: break;
    }
}

static bool strength_reduction(FeInst* inst) {

    FeInstBinop* binop = (FeInstBinop*) inst;

    switch (inst->kind) {
    case FE_INST_UMUL:

        if (binop->rhs->kind == FE_INST_CONST && is_const_power_of_two(binop->rhs)) {
            // x * const = x << log2(const)
            FeInstConst* log2const = (FeInstConst*) fe_inst_const(binop->base.bb->function);
            log2const->base.type = binop->rhs->type;
            log2const->i64 = ((FeInstConst*)binop->rhs)->i64;
            fe_insert_before(binop->base.bb, (FeInst*)log2const, inst);
            convert_to_log2((FeInst*)log2const);
            binop->rhs = (FeInst*)log2const;
            inst->kind = FE_INST_SHL;
            return true;
        }
    default:
        break;
    }
    return false;
}

void run_pass_algsimp(FeModule* mod) {
    da(FeInstPTR) worklist = {0};
    da_init(&worklist, 64);

    for_range(fi, 0, mod->functions_len) {
        FeFunction* f = mod->functions[fi];
        da_clear(&worklist);

        for_range(bi, 0, f->blocks.len) {
            FeBasicBlock* bb = f->blocks.at[bi];
            for_range(ii, 0, bb->len) {
                if (bb->at[ii]->kind != FE_INST_ELIMINATED);
                da_append(&worklist, bb->at[ii]);
            }
        }

        while (worklist.len != 0) {
            FeInst* inst = da_pop(&worklist);

            FeInst* new_inst;

            // first try to identity-reduce
            // then try to constant-evaluate
            // then try to strength-reduce

            // this is because identity reduction can catch some cases
            // that fall into constant evaluation. constant evaluation
            // creates a new FeInst, where identity reduction generally
            // does not. strength reduction is done last because some
            // strength reduction cases would also fall into constant 
            // evaluation so its better to const-eval it instead of 
            // strength-reducing AND THEN const-eval

            bool needs_inserting = false;
            if (inst != (new_inst = identity_reduction(inst, &needs_inserting))){
                if (needs_inserting) fe_insert_before(inst->bb, new_inst, inst);
                fe_rewrite_uses(f, inst, new_inst);
                inst->kind = FE_INST_ELIMINATED;
                fe_add_uses_to_worklist(f, new_inst, &worklist);
            } else if (inst != (new_inst = const_eval(f, inst))) {
                fe_insert_before(inst->bb, new_inst, inst);
                fe_rewrite_uses(f, inst, new_inst);
                inst->kind = FE_INST_ELIMINATED;
                fe_add_uses_to_worklist(f, new_inst, &worklist);
            } else if (strength_reduction(inst)) {
                fe_add_uses_to_worklist(f, inst, &worklist);
            }
        }
    }
    da_destroy(&worklist);
}

FePass fe_pass_algsimp = {
    .name = "algsimp",
    .callback = run_pass_algsimp,
};