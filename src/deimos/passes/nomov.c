#include "deimos.h"

static u64 ir_set_usage(IR_BasicBlock* bb, IR* source, u64 start_index, IR* dest) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == IR_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        IR** ir = (IR**)bb->at[i];
        for (u64 j = sizeof(IR)/sizeof(IR*); j <= ir_sizes[bb->at[i]->tag]/sizeof(IR*); j++) {
            if (ir[j] == source) {
                ir[j] = dest;
                return i;
            }
        }
    }
    return UINT64_MAX;
}

static void set_uses_of(IR_Function* f, IR* source, IR* dest) {
    // we probably dont need to search the whole function but whatever;
    FOR_URANGE(i, 0, f->blocks.len) {
        IR_BasicBlock* bb = f->blocks.at[i];
        u64 next_usage = ir_set_usage(bb, source, 0, dest);
        while (next_usage != UINT64_MAX) {
            next_usage = ir_set_usage(bb, source, next_usage+1, dest);
        }
    }
}

IR_Module* ir_pass_nomov(IR_Module* mod) {
    FOR_URANGE(i, 0, mod->functions_len) {
        FOR_URANGE(j, 0, mod->functions[i]->blocks.len) {
            IR_BasicBlock* bb = mod->functions[i]->blocks.at[j];

            FOR_URANGE(k, 0, bb->len) {
                if (bb->at[k]->tag != IR_MOV) continue;

                set_uses_of(mod->functions[i], bb->at[k], ((IR_Mov*)bb->at[k])->source);
                bb->at[k]->tag = IR_ELIMINATED;
            }

        }
    }
    return mod;
}