#include "iron/iron.h"

#include "short_traits.h"

// verify - IR correctness verification pass
// only built to work on generic insts

static void inst_local_check(FeFunc* f, FeInst* inst) {
    const FeTarget* target = f->mod->target;

    const FeTrait traits = fe_inst_traits(inst->kind);
    const char* inst_name = fe_inst_name(target, inst->kind);
    
    if (!inst_name) {
        FE_CRASH("inst kind %d is not recognized", inst->kind);
    }

    if ((traits & TERM) && inst->next->kind != FE__BOOKEND) {
        FE_CRASH("inst %s is a terminator but is not last in block", inst_name);
    }
    if (!(traits & TERM) && inst->next->kind == FE__BOOKEND) {
        FE_CRASH("inst %s is not a terminator but is last in block", inst_name);
    }
}

void fe_verify_func(FeFunc* f) {
    FE_CRASH("ugh");
}
