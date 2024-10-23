#include "iron/iron.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){                            \
                                               .function_of_origin = __func__,    \
                                               .message = (msg),                  \
                                               .severity = FE_MSG_SEVERITY_FATAL, \
                                           })

static void verify_basic_block(FeModule* m, FeFunction* fn, FeBasicBlock* bb, bool entry) {
    if (bb->start->kind == FE_IR_BOOKEND) {
        FE_FATAL(m, "basic blocks cannot be empty");
    }

    if (!fe_is_ir_terminator(bb->end)) {
        FE_FATAL(m, "basic blocks must end with a terminator");
    }

    for_fe_ir(inst, *bb) {
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
                FE_FATAL(m, "terminator must be at the end of a basic block");
            }
            break;
        default:
            break;
        }
    }
}

static void verify_function(FeFunction* f) {
    if (f->blocks.len == 0) {
        FE_FATAL(f->mod, "functions must have at least one basic block");
    }
    foreach (FeBasicBlock* bb, f->blocks) {
        verify_basic_block(f->mod, f, bb, count == 0);
    }
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