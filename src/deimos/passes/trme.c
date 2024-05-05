#include "deimos.h"

/* pass "trme" - trivial redundant memory op eliminations

    transforms store-load patterns into store-mov patterns (eliminating redundant loads)
    as well as eliminating stackallocs with only stores as uses

*/

// this all will eventually be replaced by the "stackpromote" pass
// this needs to be rewritten because i dont actually think its sound
// because of potential pointer aliasing problems

da_typedef(IR_PTR);

static IR_Store* last_local_store_to_loc(IR_BasicBlock* bb, IR* location, u64 from) {
    if (location == NULL) return NULL;
    for (i64 i = from-1; i >= 0; i--) {
        if (bb->at[i]->tag == IR_STORE && ((IR_Store*)bb->at[i])->location == location) {
            return (IR_Store*) bb->at[i];
        }
    }
    return NULL;
}

static u64 ir_get_usage(IR_BasicBlock* bb, IR* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->tag == IR_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        IR** ir = (IR**)bb->at[i];
        for (u64 j = sizeof(IR)/sizeof(IR*); j <= ir_sizes[bb->at[i]->tag]/sizeof(IR*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

// this is horrible code, but like i said, it will be replaced by stackpromote
IR_Module* ir_pass_trme(IR_Module* mod) {

    da(IR_PTR) store_elim_list;
    da_init(&store_elim_list, 4);

    for (u64 i = 0; i < mod->functions_len; i++) {
        IR_Function* f = mod->functions[i];

        // transform redundant loads into movs
        for (u64 j = 0; j < f->blocks.len; j++) {
            IR_BasicBlock* bb = f->blocks.at[j];

            for (u64 inst = 0; inst < bb->len; inst++) {
                if (bb->at[inst]->tag != IR_LOAD) continue;

                IR_Load* ir = (IR_Load*) bb->at[inst];  
                IR_Store* last_store = last_local_store_to_loc(bb, ir->location, inst);

                if (last_store == NULL) continue;

                // in general, this is a BAD IDEA
                // but its fine because sizeof(IR_Load) >= sizeof(IR_Mov)
                // dont do this kinda thing unless you're
                // ABSOLUTELY SURE its going to be okay
                ir->base.tag = IR_MOV;
                ((IR_Mov*)ir)->source = last_store->value;
            }
        }

        // eliminate stackallocs with only stores as their uses
        for (u64 j = 0; j < f->blocks.len; j++) {
            IR_BasicBlock* bb = f->blocks.at[j];

            for (u64 inst = 0; inst < bb->len; inst++) {
                if (bb->at[inst]->tag != IR_STACKALLOC) continue;

                for (u64 search_bb = 0; search_bb < f->blocks.len; search_bb++) {
                    u64 next_usage = ir_get_usage(f->blocks.at[search_bb], bb->at[inst], 0);
                    while (next_usage != UINT64_MAX) {
                        IR* usage = f->blocks.at[search_bb]->at[next_usage];
                        if (usage->tag != IR_STORE) {
                            goto continue_stackalloc_scan;
                        }
                        da_append(&store_elim_list, usage);
                        next_usage = ir_get_usage(f->blocks.at[search_bb], bb->at[inst], next_usage + 1);
                    }
                }

                // if you're still around, this stackalloc is useless and can be eliminated
                bb->at[inst]->tag = IR_ELIMINATED;

                // eliminate the stores as well
                for (u64 s = 0; s < store_elim_list.len; s++) {
                    store_elim_list.at[s]->tag = IR_ELIMINATED;
                }

                continue_stackalloc_scan:
                da_clear(&store_elim_list);
            }

        }
    }

    da_destroy(&store_elim_list);
    return mod;
}

