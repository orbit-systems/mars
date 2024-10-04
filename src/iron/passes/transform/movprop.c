#include "iron/iron.h"

/* pass "movprop" - mov propogation

    eliminate mov instructions and rewrite its uses to the use the eliminated mov's source.

*/

static void run_pass_movprop(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        for_urange(j, 0, fn->blocks.len) {
            FeBasicBlock* bb = fn->blocks.at[j];

            for_fe_ir(inst, *bb) {
                if (inst->kind != FE_IR_MOV) continue;

                fe_rewrite_ir_uses(fn, inst, ((FeIrMov*)inst)->source);
                fe_remove_ir(inst);
            }

        }
    }
}

FePass fe_pass_movprop = {
    .name = "movprop",
    .callback = run_pass_movprop,
};