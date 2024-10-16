#include "iron/iron.h"

/* pass "moviphi" - insert movs between phis and their sources

    this pass is usually invoked by targets before codegen
    makes phi translation easier

*/

static void function_moviphi(FeFunction* fn) {
    for_urange(j, 0, fn->blocks.len) {
        FeBasicBlock* bb = fn->blocks.at[j];

        FeIrPhi* phi = (FeIrPhi*)bb->start;
        while (phi->base.kind == FE_IR_PHI) {
            // insert movs at the end of each sources' basic block
            for_range(i, 0, phi->len) {
                FeBasicBlock* source_bb = phi->source_BBs[i];
                FeIr* source = phi->sources[i];
                if (source->kind != FE_IR_MOV) phi->sources[i] = fe_insert_ir_before(fe_ir_mov(fn, source), source_bb->end);
            }
            phi = (FeIrPhi*)phi->base.next;
        }
    }
}

static void module_moviphi(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        function_moviphi(fn);
    }
}

FePass fe_pass_moviphi = {
    .name = "moviphi",
    .module = module_moviphi,
    .function = function_moviphi,
};