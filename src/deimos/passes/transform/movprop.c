#include "deimos.h"

/* pass "movprop" - mov propogation

    eliminate mov instructions and rewrite its uses to the use the eliminated mov's source.

*/

static u64 air_set_usage(AIR_BasicBlock* bb, AIR* source, u64 start_index, AIR* dest) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == AIR_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        AIR** ir = (AIR**)bb->at[i];
        for (u64 j = sizeof(AIR)/sizeof(AIR*); j <= air_sizes[bb->at[i]->tag]/sizeof(AIR*); j++) {
            if (ir[j] == source) {
                ir[j] = dest;
                return i;
            }
        }
    }
    return UINT64_MAX;
}

static void set_uses_of(AIR_Function* f, AIR* source, AIR* dest) {
    // we probably dont need to search the whole function but whatever;
    for_urange(i, 0, f->blocks.len) {
        AIR_BasicBlock* bb = f->blocks.at[i];
        u64 next_usage = air_set_usage(bb, source, 0, dest);
        while (next_usage != UINT64_MAX) {
            next_usage = air_set_usage(bb, source, next_usage+1, dest);
        }
    }
}

AIR_Module* air_pass_movprop(AIR_Module* mod) {
    for_urange(i, 0, mod->functions_len) {
        for_urange(j, 0, mod->functions[i]->blocks.len) {
            AIR_BasicBlock* bb = mod->functions[i]->blocks.at[j];

            for_urange(k, 0, bb->len) {
                if (bb->at[k]->tag != AIR_MOV) continue;

                set_uses_of(mod->functions[i], bb->at[k], ((AIR_Mov*)bb->at[k])->source);
                bb->at[k]->tag = AIR_ELIMINATED;
            }

        }
    }
    return mod;
}