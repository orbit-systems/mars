#include "iron/iron.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){                            \
                                               .function_of_origin = __func__,    \
                                               .message = (msg),                  \
                                               .severity = FE_REP_SEVERITY_FATAL, \
                                           })

static FeBasicBlock* entry_block;

static da(FeIrPTR) active_defs;

// checks that all uses of inst are defined and 
// non-self-referencial (with the exception of phis)
static bool is_defined(FeIr* inst) {
    for (isize i = active_defs.len - 1; i >= 0; --i) {
        if (active_defs.at[i] == inst) return true;
    }
    return true;
}


#define fatal_if_not_def(inst) if (!is_defined(inst)) FE_FATAL(m, "argument not defined yet in this control path")
static void check_defined(FeModule* m, FeIr* inst) {
    if (inst->kind == FE_IR_PHI) da_append(&active_defs, inst);

    switch (inst->kind) {
    case FE_IR_ADD:
    case FE_IR_SUB:
    case FE_IR_IMUL:
    case FE_IR_UMUL:
    case FE_IR_IDIV:
    case FE_IR_UDIV:
    case FE_IR_AND:
    case FE_IR_OR:
    case FE_IR_XOR:
    case FE_IR_SHL:
    case FE_IR_LSR:
    case FE_IR_ASR:
    case FE_IR_ULT:
    case FE_IR_UGT:
    case FE_IR_ULE:
    case FE_IR_UGE:
    case FE_IR_ILT:
    case FE_IR_IGT:
    case FE_IR_ILE:
    case FE_IR_IGE:
    case FE_IR_EQ:
    case FE_IR_NE:
        FeIrBinop* binop = (FeIrBinop*)inst;
        fatal_if_not_def(binop->lhs);
        fatal_if_not_def(binop->rhs);
        break;
    case FE_IR_STACK_STORE:
        FeIrStackStore* stack_store = (FeIrStackStore*)inst;
        fatal_if_not_def(stack_store->value);
        break;
    case FE_IR_BRANCH:
        FeIrBranch* branch = (FeIrBranch*)inst;
        fatal_if_not_def(branch->cond);
        break;

    case FE_IR_PHI:
        FeIrPhi* phi = (FeIrPhi*)inst;
        for_range(i, 0, phi->len) {
            fatal_if_not_def(phi->sources[i]);
        }
        break;
    case FE_IR_RETURN:
        FeIrReturn* ret = (FeIrReturn*)inst;
        for_range(i, 0, ret->len) {
            fatal_if_not_def(ret->sources[i]);
        }
        break;
    case FE_IR_PARAM:
    case FE_IR_STACK_ADDR:
    case FE_IR_STACK_LOAD:
    case FE_IR_CONST:
    case FE_IR_JUMP:
        break; // no inputs
    default:
        printf("---- %d\n", inst->kind);
        CRASH("unhandled in check_defined");
        break;
    }
    if (inst->kind != FE_IR_PHI) da_append(&active_defs, inst);
}

static void verify_basic_block(FeModule* m, FeFunction* fn, FeBasicBlock* bb, bool entry) {
    if (entry) entry_block = bb;

    // active def save point for rewinding
    usize savepoint = active_defs.len;

    if (bb->start->kind == FE_IR_BOOKEND) {
        FE_FATAL(m, "basic blocks must have at least one instruction");
    }

    if (!fe_is_ir_terminator(bb->end)) {
        FE_FATAL(m, "basic blocks must end with a terminator");
    }

    for_fe_ir(inst, *bb) {
        if (fe_is_ir_terminator(inst) && inst->next->kind != FE_IR_BOOKEND) {
            FE_FATAL(m, "terminator must be at the end of a basic block");
        }
        check_defined(m, inst);
        switch (inst->kind) {
        case FE_IR_PARAM:
            if (!entry || (inst->prev->kind != FE_IR_BOOKEND && inst->prev->kind != FE_IR_PARAM))
                FE_FATAL(m, "param must be at the beginning of the entry block");

            FeIrParam* param = (FeIrParam*)inst;
            if (param->index >= fn->params.len) FE_FATAL(m, "param index out of range");
            if (inst->type != fn->params.at[param->index]->type) FE_FATAL(m, "param type != param type");
            break;
        case FE_IR_PHI:
            if (inst->prev->kind != FE_IR_BOOKEND && inst->prev->kind != FE_IR_PHI) {
                FE_FATAL(m, "phi must be at the beginning of a basic block");
            }
            FeIrPhi* phi = (FeIrPhi*)inst;
            break;

        case FE_IR_RETURN:
        case FE_IR_JUMP:
        case FE_IR_BRANCH:
            if (inst->next->kind != FE_IR_BOOKEND) {
            }
            break;
        default:
            break;
        }
    }
    active_defs.len = savepoint;
}

static void reset_flags(FeFunction* f) {
    for_urange(i, 0, f->blocks.len) {
        for_fe_ir(inst, *f->blocks.at[i]) {
            inst->flags = 0;
        }
    }
}

static void verify_function(FeFunction* f) {
    da_init(&active_defs, 128);
    if (f->blocks.len == 0) {
        FE_FATAL(f->mod, "functions must have at least one basic block");
    }
    reset_flags(f);
    verify_basic_block(f->mod, f, f->blocks.at[0], true);
}

static void verify_module(FeModule* m) {
    for_range(i, 0, m->functions_len) {
        FeFunction* f = m->functions[i];
        verify_function(f);
    }
}

// fails fatally if module is not valid.
FePass fe_pass_verify = {
    .name = "verify",
    .module = verify_module,
    .function = verify_function,
};