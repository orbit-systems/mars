#include "iron/iron.h"

/* pass "moviphi" - insert movs between phis and their sources
    was originally "movify" but this is funnier

    this pass is usually invoked by targets before codegen
    makes phi translation easier
*/

static void run_pass_moviphi(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        for_urange(j, 0, fn->blocks.len) {
            FeBasicBlock* bb = fn->blocks.at[j];

            FeIrPhi* phi = (FeIrPhi*) bb->start;
            while (phi->base.kind == FE_IR_PHI) {
                // insert movs at the end of each sources' basic block
                for_range(i, 0, phi->len) {
                    FeBasicBlock* source_bb = phi->source_BBs[i];
                    FeIr* source = phi->sources[i];
                    phi->sources[i] = fe_insert_ir_before(fe_ir_mov(fn, source), source_bb->end);
                }
                phi = (FeIrPhi*) phi->base.next;
            }
        }
    }
}

FePass fe_pass_moviphi = {
    .name = "moviphi",
    .callback = run_pass_moviphi,
};