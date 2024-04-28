#include "deimos.h"

/* pass "org" - general cleanup after ir generation

    order instructions inside basic blocks such that 
    paramvals come first, then stackallocs, then the rest of the code.

*/

static void sort_instructions(IR_BasicBlock* bb) {
    u64 last_stackalloc = 0;
    u64 last_paramval = 0;

    for (u64 i = 0; i < bb->len; i++) {
        if (last_paramval >= bb->len) break;
        if (last_stackalloc >= bb->len) break;
        if (bb->at[i]->tag == IR_PARAMVAL) {
            ir_move_element(bb, last_paramval, i);
            last_paramval++;
            last_stackalloc++;
        } else if (bb->at[i]->tag == IR_STACKALLOC) {
            ir_move_element(bb, last_stackalloc, i);
            last_stackalloc++;
        }
    }
}

IR_Module* ir_pass_org(IR_Module* mod) {
    // reorg stackallocs and paramvals
    FOR_URANGE(i, 0, mod->functions_len) {
        FOR_URANGE(j, 0, mod->functions[i]->blocks.len) {
            sort_instructions(mod->functions[i]->blocks.at[j]);
        }
    }
    return mod;
}