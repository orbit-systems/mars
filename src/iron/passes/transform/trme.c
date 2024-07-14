#include "iron.h"

/* pass "trme" - trivial redundant memory op eliminations

    transforms store-load patterns into store-mov patterns (eliminating redundant loads)
    as well as eliminating stackallocs with only stores as uses

*/

// this all will eventually be replaced by the "stackpromote" pass
// this needs to be rewritten because i dont actually think its sound
// because of potential pointer aliasing problems

da_typedef(FeInstPTR);

static FeStore* last_local_store_to_loc(FeBasicBlock* bb, FeInst* location, u64 from) {
    if (location == NULL) return NULL;
    for (i64 i = from-1; i >= 0; i--) {
        if (bb->at[i]->tag == FE_INST_STORE && ((FeStore*)bb->at[i])->location == location) {
            return (FeStore*) bb->at[i];
        }
    }
    return NULL;
}

static u64 air_get_usage(FeBasicBlock* bb, FeInst* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == FE_INST_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        FeInst** ir = (FeInst**)bb->at[i];
        for (u64 j = sizeof(FeInst)/sizeof(FeInst*); j <= air_sizes[bb->at[i]->tag]/sizeof(FeInst*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

static void remove_stack_slot(FeFunction* f, FeStackObject* obj) {
    for_urange(i, 0, f->stack.len) {
        if (f->stack.at[i] == obj) {
            da_remove_at(&f->stack, i);
            return;
        }
    }
}

// this is horrible code, but like i said, it will be replaced by stackpromote
void run_pass_trme(FeModule* mod) {

    da(FeInstPTR) store_elim_list;
    da_init(&store_elim_list, 4);

    for (u64 i = 0; i < mod->functions_len; i++) {
        FeFunction* f = mod->functions[i];

        // transform redundant loads into movs
        for (u64 j = 0; j < f->blocks.len; j++) {
            FeBasicBlock* bb = f->blocks.at[j];

            for (u64 inst = 0; inst < bb->len; inst++) {
                if (bb->at[inst]->tag != FE_INST_LOAD) continue;

                FeLoad* ir = (FeLoad*) bb->at[inst];  
                FeStore* last_store = last_local_store_to_loc(bb, ir->location, inst);

                if (last_store == NULL) continue;

                // in general, this is a BAD IDEA
                // but its fine because sizeof(FeLoad) >= sizeof(FeMov)
                // dont do this kinda thing unless you're
                // ABSOLUTELY SURE its going to be okay
                ir->base.tag = FE_INST_MOV;
                ((FeMov*)ir)->source = last_store->value;
            }
        }

        // eliminate stackallocs with only stores as their uses
        for (u64 j = 0; j < f->blocks.len; j++) {
            FeBasicBlock* bb = f->blocks.at[j];

            for (u64 inst = 0; inst < bb->len; inst++) {
                if (bb->at[inst]->tag != FE_INST_STACKADDR) continue;

                for (u64 search_bb = 0; search_bb < f->blocks.len; search_bb++) {
                    u64 next_usage = air_get_usage(f->blocks.at[search_bb], bb->at[inst], 0);
                    while (next_usage != UINT64_MAX) {
                        FeInst* usage = f->blocks.at[search_bb]->at[next_usage];
                        if (usage->tag != FE_INST_STORE) {
                            goto continue_stackalloc_scan;
                        }
                        da_append(&store_elim_list, usage);
                        next_usage = air_get_usage(f->blocks.at[search_bb], bb->at[inst], next_usage + 1);
                    }
                }

                // if you're still around, this stack slot is useless and can be eliminated
                remove_stack_slot(f, ((FeStackAddr*)bb->at[inst])->object);
                bb->at[inst]->tag = FE_INST_ELIMINATED;

                // eliminate the stores as well
                for (u64 s = 0; s < store_elim_list.len; s++) {
                    store_elim_list.at[s]->tag = FE_INST_ELIMINATED;
                }

                continue_stackalloc_scan:
                da_clear(&store_elim_list);
            }

        }
    }

    da_destroy(&store_elim_list);
}

FePass air_pass_trme = {
    .name = "trme",
    .callback = run_pass_trme,
};