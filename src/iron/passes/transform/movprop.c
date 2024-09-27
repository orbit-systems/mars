#include "iron/iron.h"

/* pass "movprop" - mov propogation

    eliminate mov instructions and rewrite its uses to the use the eliminated mov's source.

*/


void run_pass_movprop(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        for_urange(j, 0, fn->blocks.len) {
            FeBasicBlock* bb = fn->blocks.at[j];

            for_inst(inst, *bb) {
                if (inst->kind != FE_INST_MOV) continue;

                fe_rewrite_uses(fn, inst, ((FeInstMov*)inst)->source);
                fe_remove(inst);
            }

        }
    }
}

FePass fe_pass_movprop = {
    .name = "movprop",
    .callback = run_pass_movprop,
};