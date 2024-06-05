#include "iron.h"

/* pass "movprop" - mov propogation

    eliminate mov instructions and rewrite its uses to the use the eliminated mov's source.

*/

static u64 air_set_usage(FeBasicBlock* bb, FeInst* source, u64 start_index, FeInst* dest) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == FE_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        FeInst** ir = (FeInst**)bb->at[i];
        for (u64 j = sizeof(FeInst)/sizeof(FeInst*); j <= air_sizes[bb->at[i]->tag]/sizeof(FeInst*); j++) {
            if (ir[j] == source) {
                ir[j] = dest;
                return i;
            }
        }
    }
    return UINT64_MAX;
}

static void set_uses_of(FeFunction* f, FeInst* source, FeInst* dest) {
    // we probably dont need to search the whole function but whatever;
    for_urange(i, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[i];
        u64 next_usage = air_set_usage(bb, source, 0, dest);
        while (next_usage != UINT64_MAX) {
            next_usage = air_set_usage(bb, source, next_usage+1, dest);
        }
    }
}

void run_pass_movprop(FeModule* mod) {
    for_urange(i, 0, mod->ir_module->functions_len) {
        for_urange(j, 0, mod->ir_module->functions[i]->blocks.len) {
            FeBasicBlock* bb = mod->ir_module->functions[i]->blocks.at[j];

            for_urange(k, 0, bb->len) {
                if (bb->at[k]->tag != FE_MOV) continue;

                set_uses_of(mod->ir_module->functions[i], bb->at[k], ((FeMov*)bb->at[k])->source);
                bb->at[k]->tag = FE_ELIMINATED;
            }

        }
    }
}

FePass air_pass_movprop = {
    .name = "movprop",
    .callback = run_pass_movprop,
};