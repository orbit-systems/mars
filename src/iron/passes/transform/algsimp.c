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
        ~(~x)      -> x
        -(-x)      -> x
        
        (these only applies to bool):
            x && false -> false         
            x && true  -> x
            x || false -> x
            x || true  -> true

    reassociation:
        (x + 1) + 2  ->  x + (1 + 2)
        (x * 1) * 2  ->  x * (1 * 2)
        (x & 1) & 2  ->  x & (1 & 2)
        (x | 1) | 2  ->  x | (1 | 2)
        xor is also associative
*/

// if possible, replace the contents of target with source.
// static bool replace_with(FeIr* target, FeIr* source) {
//     if (fe_inst_sizes[target->kind] < fe_inst_sizes[source->kind]) {
//         // source node is too big, we cant stuff it in
//         return false;
//     }

//     memcpy(target, source, fe_inst_sizes[source->kind]);
//     return true;
// }

#define int_operate(op, dest, src1, src2) \
    switch (dest->base.type) {\
    case FE_TYPE_I64:  dest->i64  = src1->i64  op src2->i64;  break;\
    case FE_TYPE_I32:  dest->i32  = src1->i32  op src2->i32;  break;\
    case FE_TYPE_I16:  dest->i16  = src1->i16  op src2->i16;  break;\
    case FE_TYPE_I8:   dest->i64  = src1->i8   op src2->i8;   break;\
    case FE_TYPE_BOOL: dest->bool = src1->bool op src2->bool; break;\
    }

#define uint_operate(op, dest, src1, src2) \
    switch (dest->base.type) {\
    case FE_TYPE_I64:  dest->i64  = (u64) src1->i64  op (u64) src2->i64;  break;\
    case FE_TYPE_I32:  dest->i32  = (u32) src1->i32  op (u32) src2->i32;  break;\
    case FE_TYPE_I16:  dest->i16  = (u16) src1->i16  op (u16) src2->i16;  break;\
    case FE_TYPE_I8:   dest->i64  = (u8)  src1->i8   op (u8)  src2->i8;   break;\
    case FE_TYPE_BOOL: dest->bool =       src1->bool op       src2->bool; break;\
    }

static FeIr* const_eval_binop(FeFunction* f, FeIr* inst) {
    if (!(_FE_IR_BINOP_BEGIN < inst->kind && inst->kind < _FE_BINOP_END)) {
        return inst;
    }
    FeIrBinop* binop = (FeIrBinop*) inst;

    if (binop->lhs->kind != FE_IR_CONST || binop->rhs->kind != FE_IR_CONST) {
        return inst;
    }

    FeIrConst* src1 = (FeIrConst*) binop->lhs;
    FeIrConst* src2 = (FeIrConst*) binop->rhs;
    FeIrConst* lc   = (FeIrConst*) fe_ir_const(f, inst->type);

    switch (inst->kind) {
    case FE_IR_ADD:   int_operate(+, lc, src1, src2); break;
    case FE_IR_SUB:   int_operate(-, lc, src1, src2); break;
    case FE_IR_IMUL:  int_operate(*, lc, src1, src2); break;
    case FE_IR_UMUL: uint_operate(*, lc, src1, src2); break;
    case FE_IR_IDIV:  int_operate(/, lc, src1, src2); break;
    case FE_IR_UDIV: uint_operate(/, lc, src1, src2); break;
    case FE_IR_AND:  uint_operate(&, lc, src1, src2); break;
    case FE_IR_OR:   uint_operate(|, lc, src1, src2); break;
    case FE_IR_XOR:  uint_operate(^, lc, src1, src2); break;
    case FE_IR_SHL:  uint_operate(<<, lc, src1, src2); break;
    case FE_IR_ASR:   int_operate(>>, lc, src1, src2); break;
    case FE_IR_LSR:  uint_operate(>>, lc, src1, src2); break;
    default:
        break;
    }
    
    return (FeIr*) lc;
}


static FeIr* const_eval(FeFunction* f, FeIr* inst) {
    return const_eval_binop(f, inst);
}

static bool is_const_one(FeIr* inst) {
    if (inst->kind != FE_IR_CONST) return false;

    FeIrConst* lc = (FeIrConst*) inst;
    switch (lc->base.type) {
    case FE_TYPE_I64: return lc->i64 == (i64) 1;
    case FE_TYPE_I32: return lc->i32 == (i32) 1;
    case FE_TYPE_I16: return lc->i16 == (i16) 1;
    case FE_TYPE_I8:  return lc->i8  == (i8 ) 1;
    case FE_TYPE_F64: return lc->f64 == (f64) 1;
    case FE_TYPE_F32: return lc->f32 == (f32) 1;
    case FE_TYPE_F16: return lc->f16 == (f16) 1;
    default: break;
    }
    return false;
}

static bool is_const_zero(FeIr* inst) {
    if (inst->kind != FE_IR_CONST) return false;

    FeIrConst* lc = (FeIrConst*) inst;
    switch (lc->base.type) {
    case FE_TYPE_I64: return lc->i64 == (i64) 0;
    case FE_TYPE_I32: return lc->i32 == (i32) 0;
    case FE_TYPE_I16: return lc->i16 == (i16) 0;
    case FE_TYPE_I8:  return lc->i8  == (i8 ) 0;
    case FE_TYPE_F64: return lc->f64 == (f64) 0;
    case FE_TYPE_F32: return lc->f32 == (f32) 0;
    case FE_TYPE_F16: return lc->f16 == (f16) 0;
    default: break;
    }
    return false;
}

static FeIr* identity_reduction(FeIr* inst, bool* needs_inserting) {

    FeIrBinop* binop = (FeIrBinop*) inst;
    FeIrUnop* unop = (FeIrUnop*) inst;

    switch (inst->kind) {
    case FE_IR_ADD: // x + 0  -> x
    case FE_IR_SUB: // x - 0  -> x
    case FE_IR_OR:  // x | 0  -> x
    case FE_IR_XOR: // x ^ 0  -> x
    case FE_IR_SHL: // x << 0 -> x
    case FE_IR_LSR: // x >> 0 -> x
    case FE_IR_ASR: // x >> 0 -> x
        if (is_const_zero(binop->rhs)) {
            return binop->lhs;
        }
        break;
    case FE_IR_UMUL:
    case FE_IR_IMUL:
        if (is_const_zero(binop->rhs)) {
            return binop->rhs; // x * 0 -> 0
        }
        if (is_const_one(binop->rhs)) {
            return binop->lhs; // x * 1 -> x
        }
        break;
    case FE_IR_NOT:
    case FE_IR_NEG:
        if (unop->source->kind == inst->kind) {
            // !(!x) -> x
            // -(-x) -> x
            return ((FeIrUnop*)unop->source)->source;
        }
        break;
    default: break;
    }
    return inst;
}

static bool is_const_power_of_two(FeIr* inst) {
    FeIrConst* lc = (FeIrConst*) inst;
    switch (lc->base.type) {
    case FE_TYPE_I64: return lc->i64 != 0 && (lc->i64 & (lc->i64 - 1)) == 0;
    case FE_TYPE_I32: return lc->i32 != 0 && (lc->i32 & (lc->i32 - 1)) == 0;
    case FE_TYPE_I16: return lc->i16 != 0 && (lc->i16 & (lc->i16 - 1)) == 0;
    case FE_TYPE_I8:  return lc->i8  != 0 && (lc->i8  & (lc->i8  - 1)) == 0;
    default: break;
    }
    return false;
}

static void convert_to_log2(FeIr* inst) {
    FeIrConst* lc = (FeIrConst*) inst;
    switch (lc->base.type) {
    case FE_TYPE_I64:lc->i64 = 8*sizeof(lc->i64) - __builtin_clzll(lc->i64) - 1; break;
    case FE_TYPE_I32:lc->i32 = 8*sizeof(lc->i64) - __builtin_clzll(lc->i32) - 1; break;
    case FE_TYPE_I16:lc->i16 = 8*sizeof(lc->i64) - __builtin_clzll(lc->i16) - 1; break;
    case FE_TYPE_I8: lc->i8  = 8*sizeof(lc->i64) - __builtin_clzll(lc->i8 ) - 1; break;
    default: break;
    }
}

static bool strength_reduction(FeFunction* fn, FeIr* inst) {

    FeIrBinop* binop = (FeIrBinop*) inst;

    switch (inst->kind) {
    case FE_IR_UMUL:

        if (binop->rhs->kind == FE_IR_CONST && is_const_power_of_two(binop->rhs)) {
            // x * const = x << log2(const)
            FeIrConst* log2const = (FeIrConst*) fe_ir_const(fn, binop->rhs->type);
            log2const->i64 = ((FeIrConst*)binop->rhs)->i64;
            fe_insert_ir_before((FeIr*)log2const, inst);
            convert_to_log2((FeIr*)log2const);
            binop->rhs = (FeIr*)log2const;
            inst->kind = FE_IR_SHL;
            return true;
        }
    default:
        break;
    }
    return false;
}

da(FeIrPTR) worklist = {0};

static void function_algsimp(FeFunction* fn) {
    if (worklist.at == NULL) da_init(&worklist, 128);
    da_clear(&worklist);

    for_range(bi, 0, fn->blocks.len) {
            FeBasicBlock* bb = fn->blocks.at[bi];
            for_fe_ir(inst, *bb) {
                da_append(&worklist, inst);
            }
        }

        while (worklist.len != 0) {
            FeIr* inst = da_pop(&worklist);

            FeIr* new_inst;

            // first try to identity-reduce
            // then try to constant-evaluate
            // then try to strength-reduce

            // this is because identity reduction can catch some cases
            // that fall into constant evaluation. constant evaluation
            // creates a new FeIr, where identity reduction generally
            // does not. strength reduction is done last because some
            // strength reduction cases would also fall into constant 
            // evaluation so its better to const-eval it instead of 
            // strength-reducing AND THEN const-eval

            bool needs_inserting = false;
            if (inst != (new_inst = identity_reduction(inst, &needs_inserting))){
                if (needs_inserting) fe_insert_ir_before(new_inst, inst);
                fe_rewrite_ir_uses(fn, inst, new_inst);
                fe_remove_ir(inst);
                fe_add_ir_uses_to_worklist(fn, new_inst, &worklist);
            } else if (inst != (new_inst = const_eval(fn, inst))) {
                fe_insert_ir_before(new_inst, inst);
                fe_rewrite_ir_uses(fn, inst, new_inst);
                fe_remove_ir(inst);
                fe_add_ir_uses_to_worklist(fn, new_inst, &worklist);
            } else if (strength_reduction(fn, inst)) {
                fe_add_ir_uses_to_worklist(fn, inst, &worklist);
            }
        }
}

static void module_algsimp(FeModule* mod) {
    for_range(fi, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[fi];
        function_algsimp(fn);
    }
}

FePass fe_pass_algsimp = {
    .name = "algsimp",
    .module = module_algsimp,
    .function = function_algsimp,
};