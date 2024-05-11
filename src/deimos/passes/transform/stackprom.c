#include "deimos.h"

/* pass "stackprom" - promote stackallocs to registers

*/

da_typedef(IR_PTR);

static u64 get_usage(IR_BasicBlock* bb, IR* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == IR_ELIMINATED) continue;
        IR** ir = (IR**)bb->at[i];
        for (u64 j = sizeof(IR)/sizeof(IR*); j <= ir_sizes[bb->at[i]->tag]/sizeof(IR*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

static bool is_promotable(IR_Function* f, IR* stackalloc) {
    if (stackalloc->tag != IR_STACKALLOC) return false;

    // collect uses
    da(IR_PTR) uses = {0};
    da_init(&uses, 5);
    // for every basic block, search the blocks for usages of the stackalloc.
    FOR_URANGE(block, 0, f->blocks.len) {

        IR_BasicBlock* bb = f->blocks.at[block];

        for (u64 use = get_usage(bb, stackalloc, 0); use != UINT64_MAX; use = get_usage(bb, stackalloc, use)) {
            IR* use_ir = bb->at[use];
            if (use_ir->tag != IR_LOAD && use_ir->tag != IR_STORE) {
                goto return_false;
            }
        }


    }

    return_false:
        da_destroy(&uses);
        return false;
}

da(IR_PTR) alloca_list = {0};

static void stackprom_f(IR_Function* f) {
    da_clear(&alloca_list);

    // add stackallocs to the list
    FOR_URANGE(b, 0, f->blocks.len) {
        FOR_URANGE(i, 0, f->blocks.at[b]->len) {
            IR* inst = f->blocks.at[b]->at[i];
            if (inst->tag == IR_STACKALLOC) {
                da_append(&alloca_list, inst);
            }
        }
    }

    TODO("");
}

IR_Module* ir_pass_stackprom(IR_Module* mod) {
    if (alloca_list.at == NULL) {
        da_init(&alloca_list, 4);
    }

    FOR_URANGE(i, 0, mod->functions_len) {
        stackprom_f(mod->functions[i]);
    }

    da_destroy(&alloca_list);
    alloca_list = (da(IR_PTR)){0};
    return mod;
}