#include "deimos.h"

/* pass "canon" - general cleanup & canonicalization

    order instructions inside basic blocks such that 
    paramvals come first, then stackallocs, then the rest of the code.

    reorder applicable commutative instructions into (register, constant) form.

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

static void canonicalize(IR* ir) {
    switch (ir->tag) {
    case IR_ADD:
    case IR_MUL:
        IR_BinOp* binop = (IR_BinOp*) ir;
        if (binop->lhs->tag == IR_CONST && binop->rhs->tag != IR_CONST) {
            void* temp = binop->lhs;
            binop->lhs = binop->rhs;
            binop->rhs = temp;
        }
        break;
    default:
        break;
    }
}

IR_Module* ir_pass_canon(IR_Module* mod) {
    // reorg stackallocs and paramvals
    for_urange(i, 0, mod->functions_len) {
        for_urange(j, 0, mod->functions[i]->blocks.len) {
            IR_BasicBlock* bb = mod->functions[i]->blocks.at[j];
            sort_instructions(bb);

            for_urange(inst, 0, bb->len) {
                canonicalize(bb->at[inst]);
            }
        }
    }
    return mod;
}