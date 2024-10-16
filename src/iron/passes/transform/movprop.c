#include "iron/iron.h"

/* pass "movprop" - mov propogation

    eliminate mov instructions and rewrite its uses to the use the eliminated mov's source.

*/

static void function_movprop(FeFunction* fn) {
    for_urange(j, 0, fn->blocks.len) {
        FeBasicBlock* bb = fn->blocks.at[j];

        for_fe_ir(inst, *bb) {
            if (inst->kind != FE_IR_MOV) continue;

            fe_rewrite_ir_uses(fn, inst, ((FeIrMov*)inst)->source);
            fe_remove_ir(inst);
        }
    }
}

static void module_movprop(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        function_movprop(fn);
    }
}

FePass fe_pass_movprop = {
    .name = "movprop",
    .module = module_movprop,
    .function = function_movprop,
};