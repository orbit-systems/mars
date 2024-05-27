#include "deimos.h"

/* pass "canon" - general cleanup & canonicalization

    order instructions inside basic blocks such that 
    paramvals come first, then stackallocs, then the rest of the code.

    reorder applicable commutative instructions into (register, constant) form.

*/

static void sort_instructions(AIR_BasicBlock* bb) {
    u64 last_stackalloc = 0;
    u64 last_paramval = 0;

    for (u64 i = 0; i < bb->len; i++) {
        if (last_paramval >= bb->len) break;
        if (last_stackalloc >= bb->len) break;
        if (bb->at[i]->tag == AIR_PARAMVAL) {
            air_move_element(bb, last_paramval, i);
            last_paramval++;
            last_stackalloc++;
        } else if (bb->at[i]->tag == AIR_STACKALLOC) {
            air_move_element(bb, last_stackalloc, i);
            last_stackalloc++;
        }
    }
}

static void canonicalize(AIR* ir) {
    switch (ir->tag) {
    case AIR_ADD:
    case AIR_MUL:
        AIR_BinOp* binop = (AIR_BinOp*) ir;
        if (binop->lhs->tag == AIR_CONST && binop->rhs->tag != AIR_CONST) {
            void* temp = binop->lhs;
            binop->lhs = binop->rhs;
            binop->rhs = temp;
        }
        break;
    default:
        break;
    }
}

AIR_Module* air_pass_canon(AIR_Module* mod) {
    // reorg stackallocs and paramvals
    for_urange(i, 0, mod->functions_len) {
        for_urange(j, 0, mod->functions[i]->blocks.len) {
            AIR_BasicBlock* bb = mod->functions[i]->blocks.at[j];
            sort_instructions(bb);

            for_urange(inst, 0, bb->len) {
                canonicalize(bb->at[inst]);
            }
        }
    }
    return mod;
}