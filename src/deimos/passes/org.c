#include "deimos.h"
//this pass moves all stack allocations to the TOP of the IR, so that targets can extract stack frame info

//this pass uses qsort to sort each IR_Function and each IR_BasicBlock

//  in-place IR element move (always from > to)
// 	take out index 4, move it to index 1)
//	a b c d e f : save element 4
//	a b c d   f
//	a   b c d f : memmove 
//	a e b c d f : store element 4
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