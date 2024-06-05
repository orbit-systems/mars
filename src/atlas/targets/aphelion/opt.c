#include "atlas.h"
#include "atlas/targets/aphelion/aphelion.h"

static void passfunc_aphelion_movopt(AtlasModule* mod) {
    AsmModule* m = mod->asm_module;
    
    for_urange(i, 0, m->functions_len) {
        for_urange(j, 0, m->functions[i]->num_blocks) {
            for_urange(k, 0, m->functions[i]->blocks[j]->len) {
                AsmInst* inst = m->functions[i]->blocks[j]->at[k];
                if (inst->template == &aphelion_instructions[APHEL_INST_MOV]) {
                    if (inst->ins[0]->real != ATLAS_PHYS_UNASSIGNED && inst->ins[0]->real == inst->outs[0]->real) {
                        m->functions[i]->blocks[j]->at[k]->template = NULL;
                    }
                }
            }
        }
    }
}

AtlasPass pass_aphelion_movopt = {
    .callback = passfunc_aphelion_movopt,
    .name = "aphelion::movopt"
};