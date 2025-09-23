#include "iron/iron.h"

// alias analysis solver.

// returns last memory effect.
FeInst* fe_solve_alias_space(FeFunc* f, u32 alias_space) {
    FE_ASSERT(f->entry_block == f->last_block);

    FeBlock* block = f->entry_block;

    FeInst* last_def = 0;

    for_inst(inst, block) {
        bool is_def = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_DEF);
        bool is_use = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_USE);
        
        if (!is_def && !is_use) {
            continue;
        }

        switch (inst->kind) {
        case FE_STORE:
        case FE_LOAD:
            if (fe_extra(inst, FeInstMemop)->alias_space != alias_space) {
                continue;
            }
            break;
        default:
            break;
        }

        if (is_use) {
            fe_set_input(f, inst, 0, last_def);
        }

        if (is_def) {
            last_def = inst;
        }
    }
    return last_def;
}
