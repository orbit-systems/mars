#include "atlas.h"

/* pass "trme" - trivial redundant memory op eliminations

    transforms store-load patterns into store-mov patterns (eliminating redundant loads)
    as well as eliminating stackallocs with only stores as uses

*/

// this all will eventually be replaced by the "stackpromote" pass
// this needs to be rewritten because i dont actually think its sound
// because of potential pointer aliasing problems

da_typedef(AIR_PTR);

static AIR_Store* last_local_store_to_loc(AIR_BasicBlock* bb, AIR* location, u64 from) {
    if (location == NULL) return NULL;
    for (i64 i = from-1; i >= 0; i--) {
        if (bb->at[i]->tag == AIR_STORE && ((AIR_Store*)bb->at[i])->location == location) {
            return (AIR_Store*) bb->at[i];
        }
    }
    return NULL;
}

static u64 air_get_usage(AIR_BasicBlock* bb, AIR* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == AIR_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        AIR** ir = (AIR**)bb->at[i];
        for (u64 j = sizeof(AIR)/sizeof(AIR*); j <= air_sizes[bb->at[i]->tag]/sizeof(AIR*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

// this is horrible code, but like i said, it will be replaced by stackpromote
void run_pass_trme(AtlasModule* mod) {

    da(AIR_PTR) store_elim_list;
    da_init(&store_elim_list, 4);

    for (u64 i = 0; i < mod->ir_module->functions_len; i++) {
        AIR_Function* f = mod->ir_module->functions[i];

        // transform redundant loads into movs
        for (u64 j = 0; j < f->blocks.len; j++) {
            AIR_BasicBlock* bb = f->blocks.at[j];

            for (u64 inst = 0; inst < bb->len; inst++) {
                if (bb->at[inst]->tag != AIR_LOAD) continue;

                AIR_Load* ir = (AIR_Load*) bb->at[inst];  
                AIR_Store* last_store = last_local_store_to_loc(bb, ir->location, inst);

                if (last_store == NULL) continue;

                // in general, this is a BAD IDEA
                // but its fine because sizeof(AIR_Load) >= sizeof(AIR_Mov)
                // dont do this kinda thing unless you're
                // ABSOLUTELY SURE its going to be okay
                ir->base.tag = AIR_MOV;
                ((AIR_Mov*)ir)->source = last_store->value;
            }
        }

        // eliminate stackallocs with only stores as their uses
        for (u64 j = 0; j < f->blocks.len; j++) {
            AIR_BasicBlock* bb = f->blocks.at[j];

            for (u64 inst = 0; inst < bb->len; inst++) {
                if (bb->at[inst]->tag != AIR_STACKALLOC) continue;

                for (u64 search_bb = 0; search_bb < f->blocks.len; search_bb++) {
                    u64 next_usage = air_get_usage(f->blocks.at[search_bb], bb->at[inst], 0);
                    while (next_usage != UINT64_MAX) {
                        AIR* usage = f->blocks.at[search_bb]->at[next_usage];
                        if (usage->tag != AIR_STORE) {
                            goto continue_stackalloc_scan;
                        }
                        da_append(&store_elim_list, usage);
                        next_usage = air_get_usage(f->blocks.at[search_bb], bb->at[inst], next_usage + 1);
                    }
                }

                // if you're still around, this stackalloc is useless and can be eliminated
                bb->at[inst]->tag = AIR_ELIMINATED;

                // eliminate the stores as well
                for (u64 s = 0; s < store_elim_list.len; s++) {
                    store_elim_list.at[s]->tag = AIR_ELIMINATED;
                }

                continue_stackalloc_scan:
                da_clear(&store_elim_list);
            }

        }
    }

    da_destroy(&store_elim_list);
}

AtlasPass air_pass_trme = {
    .name = "trme",
    .callback = run_pass_trme,
};