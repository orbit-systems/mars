#include "deimos.h"

void transcribe_w_no_elims(IR_BasicBlock* bb) {
    
    u64 place = 0;
    FOR_URANGE(i, 0, bb->len) {
        IR* ir = bb->at[i];
        if (ir == NULL || ir->tag == IR_ELIMINATED) continue;

        if (place == i) {
            place++;
            continue;
        }

        bb->at[place++] = ir;
    }
    bb->len = place;
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