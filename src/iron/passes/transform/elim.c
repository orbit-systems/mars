#include "iron/iron.h"

/* pass "elim" - remove instructions marked eliminated

    move non-FE_INST_ELIMINATED instructions to the top of the basic block
    and set the length of the basic block to exclude the extra instructions.

*/

static void transcribe_w_no_elims(FeBasicBlock* bb) {
    
    u64 place = 0;
    for_urange(i, 0, bb->len) {
        FeInst* ir = bb->at[i];
        if (ir == NULL || ir->tag == FE_INST_ELIMINATED) continue;

        if (place == i) {
            place++;
            continue;
        }

        bb->at[place++] = ir;
    }
    bb->len = place;
}

void run_pass_elim(FeModule* mod) {
    for_urange(i, 0, mod->functions_len) {
        for_urange(j, 0, mod->functions[i]->blocks.len) {
            FeBasicBlock* bb = mod->functions[i]->blocks.at[j];
            transcribe_w_no_elims(bb);
        }
    }
}

FePass air_pass_elim = {
    .name = "elim",
    .callback = run_pass_elim,
};