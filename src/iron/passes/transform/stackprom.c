#include "iron.h"

/* pass "stackprom" - promote stackallocs to registers

*/

da_typedef(FeInstPTR);

static u64 get_usage(FeBasicBlock* bb, FeInst* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == FE_ELIMINATED) continue;
        FeInst** ir = (FeInst**)bb->at[i];
        for (u64 j = sizeof(FeInst)/sizeof(FeInst*); j <= air_sizes[bb->at[i]->tag]/sizeof(FeInst*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

static bool is_promotable(FeFunction* f, FeInst* stackalloc) {
    if (stackalloc->tag != FE_STACKOFFSET) return false;

    // collect uses
    da(FeInstPTR) uses = {0};
    da_init(&uses, 5);
    // for every basic block, search the blocks for usages of the stackalloc.
    for_urange(block, 0, f->blocks.len) {

        FeBasicBlock* bb = f->blocks.at[block];

        for (u64 use = get_usage(bb, stackalloc, 0); use != UINT64_MAX; use = get_usage(bb, stackalloc, use)) {
            FeInst* use_ir = bb->at[use];
            if (use_ir->tag != FE_LOAD && use_ir->tag != FE_STORE) {
                goto return_false;
            }
        }


    }

    return_false:
        da_destroy(&uses);
        return false;
}

da(FeInstPTR) alloca_list = {0};

static void stackprom_f(FeFunction* f) {
    da_clear(&alloca_list);

    // add stackallocs to the list
    for_urange(b, 0, f->blocks.len) {
        for_urange(i, 0, f->blocks.at[b]->len) {
            FeInst* inst = f->blocks.at[b]->at[i];
            if (inst->tag == FE_STACKOFFSET) {
                da_append(&alloca_list, inst);
            }
        }
    }

    TODO("");
}

void run_pass_stackprom(FeModule* mod) {
    if (alloca_list.at == NULL) {
        da_init(&alloca_list, 4);
    }

    for_urange(i, 0, mod->ir_module->functions_len) {
        stackprom_f(mod->ir_module->functions[i]);
    }

    da_destroy(&alloca_list);
    alloca_list = (da(FeInstPTR)){0};
}