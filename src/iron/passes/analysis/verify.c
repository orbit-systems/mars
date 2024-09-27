#include "iron/iron.h"

#define FE_FATAL(m, msg) fe_push_message(m, (FeMessage){ \
    .function_of_origin = __func__,\
    .message = (msg),\
    .severity = FE_MSG_SEVERITY_FATAL, \
})

static void verify_basic_block(FeModule* m, FeBasicBlock* bb, bool entry) {
    if (bb->start->kind == FE_INST_BOOKEND) {
        FE_FATAL(m, "basic blocks cannot be empty");
    }

    if (!fe_inst_is_terminator(bb->end)) {
        FE_FATAL(m, "basic blocks must end with a terminator instruction");
    }

    for_fe_inst(inst, *bb) {
        
    }
}

static void verify_function(FeModule* m, FeFunction* f) {

}

static void verify_module(FeModule* m) {

}

// fails fatally if module is not valid.
FePass fe_pass_verify = {
    .name = "verify",
    .callback = verify_module,
};