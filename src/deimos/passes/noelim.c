#include "deimos.h"

void transcribe_w_no_elims(IR_BasicBlock* bb) {
    
    // pre-scan
    u64 non_elim_count = 0;
    FOR_URANGE(i, 0, bb->len) {
        if (bb->at[i]->tag != IR_ELIMINATED) non_elim_count++;
    }

    if (non_elim_count == bb->len) return;

    IR_BasicBlock new_bb;
    da_init(&new_bb, non_elim_count);

    FOR_URANGE(i, 0, bb->len) {
        if (bb->at[i]->tag != IR_ELIMINATED) {
            ir_add(&new_bb, bb->at[i]);
        }
    }

    da_destroy(bb);

    bb->at = new_bb.at;
    bb->cap = new_bb.cap;
    bb->len = new_bb.len;
}

IR_Module* ir_pass_noelim(IR_Module* mod) {
    FOR_URANGE(i, 0, mod->functions_len) {
        FOR_URANGE(j, 0, mod->functions[i]->blocks.len) {
            IR_BasicBlock* bb = mod->functions[i]->blocks.at[j];
            transcribe_w_no_elims(bb);
        }
    }
    return mod;
}