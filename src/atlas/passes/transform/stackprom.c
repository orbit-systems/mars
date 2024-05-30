#include "atlas.h"

/* pass "stackprom" - promote stackallocs to registers

*/

da_typedef(AIR_Ptr);

static u64 get_usage(AIR_BasicBlock* bb, AIR* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == AIR_ELIMINATED) continue;
        AIR** ir = (AIR**)bb->at[i];
        for (u64 j = sizeof(AIR)/sizeof(AIR*); j <= air_sizes[bb->at[i]->tag]/sizeof(AIR*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

static bool is_promotable(AIR_Function* f, AIR* stackalloc) {
    if (stackalloc->tag != AIR_STACKALLOC) return false;

    // collect uses
    da(AIR_Ptr) uses = {0};
    da_init(&uses, 5);
    // for every basic block, search the blocks for usages of the stackalloc.
    for_urange(block, 0, f->blocks.len) {

        AIR_BasicBlock* bb = f->blocks.at[block];

        for (u64 use = get_usage(bb, stackalloc, 0); use != UINT64_MAX; use = get_usage(bb, stackalloc, use)) {
            AIR* use_ir = bb->at[use];
            if (use_ir->tag != AIR_LOAD && use_ir->tag != AIR_STORE) {
                goto return_false;
            }
        }


    }

    return_false:
        da_destroy(&uses);
        return false;
}

da(AIR_Ptr) alloca_list = {0};

static void stackprom_f(AIR_Function* f) {
    da_clear(&alloca_list);

    // add stackallocs to the list
    for_urange(b, 0, f->blocks.len) {
        for_urange(i, 0, f->blocks.at[b]->len) {
            AIR* inst = f->blocks.at[b]->at[i];
            if (inst->tag == AIR_STACKALLOC) {
                da_append(&alloca_list, inst);
            }
        }
    }

    TODO("");
}

void run_pass_stackprom(AtlasModule* mod) {
    if (alloca_list.at == NULL) {
        da_init(&alloca_list, 4);
    }

    for_urange(i, 0, mod->ir_module->functions_len) {
        stackprom_f(mod->ir_module->functions[i]);
    }

    da_destroy(&alloca_list);
    alloca_list = (da(AIR_Ptr)){0};
}