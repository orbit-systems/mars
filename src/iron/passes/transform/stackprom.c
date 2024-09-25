#include "iron/iron.h"

/* pass "stackprom" - promote stack objects to registers

*/

// static u64 get_usage(FeBasicBlock* bb, FeInst* source, u64 start_index) {
//     for (u64 i = start_index; i < bb->len; i++) {
//         if (bb->at[i]->kind == FE_INST_ELIMINATED) continue;
//         FeInst** ir = (FeInst**)bb->at[i];
//         for (u64 j = sizeof(FeInst)/sizeof(FeInst*); j <= fe_inst_sizes[bb->at[i]->kind]/sizeof(FeInst*); j++) {
//             if (ir[j] == source) return i;
//         }
//     }
//     return UINT64_MAX;
// }

// this is gonna be very inefficient, but it will work for now.

static bool candidate_for_stackprom(FeStackObject* obj, FeFunction* f) {
    foreach(FeBasicBlock* bb, f->blocks) {
        foreach(FeInst* inst, *bb) {

        }
    }
}

static void per_function(FeFunction* f) {
    
}

static void run_pass_stackprom(FeModule* m) {
    for_urange(i, 0, m->functions_len) {
        per_function(m->functions[i]);
    }
}

FePass fe_pass_stackprom = {
    .name = "stackprom",
    .callback = run_pass_stackprom,
};