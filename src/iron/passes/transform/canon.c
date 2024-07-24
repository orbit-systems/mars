#include "iron/iron.h"

/* pass "canon" - general cleanup & canonicalization

    order instructions inside basic blocks such that 
    paramvals come first, then stackallocs, then the rest of the code.

    reorder applicable commutative instructions into (register, constant) form.

*/

static void sort_instructions(FeBasicBlock* bb) {
    u64 last_paramval = 0;

    for (u64 i = 0; i < bb->len; i++) {
        if (last_paramval >= bb->len) break;
        if (bb->at[i]->kind == FE_INST_PARAMVAL) {
            fe_move(bb, last_paramval, i);
            last_paramval++;
        }
    }
}

static void canonicalize(FeInst* ir) {
    switch (ir->kind) {
    case FE_INST_ADD:
    case FE_INST_UMUL:
    case FE_INST_IMUL:
        FeInstBinop* binop = (FeInstBinop*) ir;
        if (binop->lhs->kind == FE_INST_CONST && binop->rhs->kind != FE_INST_CONST) {
            void* temp = binop->lhs;
            binop->lhs = binop->rhs;
            binop->rhs = temp;
        }
        break;
    default:
        break;
    }
}

void run_pass_canon(FeModule* mod) {
    // reorg stackallocs and paramvals
    for_urange(i, 0, mod->functions_len) {
        for_urange(j, 0, mod->functions[i]->blocks.len) {
            FeBasicBlock* bb = mod->functions[i]->blocks.at[j];
            sort_instructions(bb);

            for_urange(inst, 0, bb->len) {
                canonicalize(bb->at[inst]);
            }
        }
    }
}

FePass fe_pass_canon = {
    .name = "canon",
    .callback = run_pass_canon,
};