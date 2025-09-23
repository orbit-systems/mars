#include "common/util.h"
#include "iron/iron.h"

// various post-regalloc optimizations

static bool try_mov_elim(FeFunc* f, FeInstSet* wlist, FeInst* mov) {
    if (!fe_inst_has_trait(mov->kind, FE_TRAIT_REG_MOV_HINT)) {
        return false;
    }

    if (mov->vr_def == FE_VREG_NONE || f->vregs->at[mov->vr_def].real == FE_VREG_REAL_UNASSIGNED) {
        return false;
    }

    if (f->vregs->at[mov->vr_def].real == f->vregs->at[mov->inputs[0]->vr_def].real) {
        fe_replace_uses(f, mov, mov->inputs[0]);
        fe_inst_destroy(f, mov);
        return true;
    }
   
    return false;
}


void fe_opt_post_regalloc(FeFunc* f) {
    FeInstSet wlist;
    fe_iset_init(&wlist);

    for_blocks(block, f) {
        for_inst(inst, block) {
            fe_iset_push(&wlist, inst);
        }
    }

    while (true) {
        FeInst* inst = fe_iset_pop(&wlist);
        if (inst == nullptr) {
            break;
        }

        bool opts = false
            || try_mov_elim(f, &wlist, inst)
        ;
    }

    fe_iset_destroy(&wlist);
}
