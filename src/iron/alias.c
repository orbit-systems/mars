#include "iron/iron.h"

// calculates memory effect chains for every inst in a function
// under the assumption that everything aliases everything else
FeInst* fe_solve_mem_pessimistic(FeFunc* f) {
    FE_ASSERT(f->entry_block == f->last_block);

    FeBlock* block = f->entry_block;

    FeInst* last_def = 0;

    for_inst(inst, block) {
        bool is_def = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_DEF);
        bool is_use = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_USE);
        
        if (!is_def && !is_use) {
            continue;
        }

        // switch (inst->kind) {
        // case FE_STORE:
        // case FE_LOAD:
        //     break;
        // default:
        //     break;
        // }

        if (is_use) {
            fe_set_input(f, inst, 0, last_def);
        }

        if (is_def) {
            last_def = inst;
        }
    }
    return last_def;
}

static bool is_load_or_store(FeInst* inst) {
    return inst->kind == FE_STORE || inst->kind == FE_LOAD;
}

static usize load_or_store_width(FeInst* inst) {
    switch (inst->kind) {
    case FE_LOAD:
        return fe_ty_get_size(inst->ty, nullptr);
    case FE_STORE:
        return fe_ty_get_size(inst->inputs[2]->ty, nullptr);
    default:
        FE_CRASH("load_or_store_width called with inst that isnt a load or store");
    }
}


// return true if [x_start, x_end) overlaps with [y_start, y_end)
static inline bool ranges_overlap_right_excl(usize x_start, usize x_end, usize y_start, usize y_end) {
    return x_start < y_end && y_start < x_end;
}

typedef struct {
    FeInst* root;
    usize offset;
} PtrOffset;

static PtrOffset trace_ptr_offset(FeInst* ptr) {
    usize offset = 0;

    while (true) {
        switch (ptr->kind) {
        case FE_IADD:
            if (ptr->inputs[1]->kind != FE_CONST) {
                continue;
            }
            offset += fe_extra(ptr->inputs[1], FeInstConst)->val;
            ptr = ptr->inputs[0];
            break;
        default:
            goto done;
        }
    }
    done:

    return (PtrOffset){ptr, offset};
}

/*
    - symaddrs of different symbols will never alias each other
    - stackaddrs of different slots will never alias each other
    - pointers from a NOALIAS will never alias other pointers in a function
    - non-overlapping offsets from the same pointer will never alias each other
*/
static bool pointers_may_overlap(FeInst* ptr1, usize width1, FeInst* ptr2, usize width2) {
    // assumes a type of 'canonical' form on the IR, 
    // where constants expressions are propagated

    PtrOffset source1 = trace_ptr_offset(ptr1);
    PtrOffset source2 = trace_ptr_offset(ptr2);

    // offsets from symbol addresses
    if (source1.root->kind == FE_SYM_ADDR && source2.root->kind == FE_SYM_ADDR) {
        // not the same symbol, definitely wont alias
        if (fe_extra(source1.root, FeInstSymAddr)->sym != 
            fe_extra(source2.root, FeInstSymAddr)->sym
        ) {
            return false;
        }
        // these locations will only alias each other
        // IF their access ranges overlap.
        return ranges_overlap_right_excl(
            source1.offset, source1.offset + width1, 
            source2.offset, source2.offset + width2
        );
    }

    if (source1.root->kind == FE_STACK_ADDR && source2.root->kind == FE_STACK_ADDR) {
        // not the same symbol, definitely wont alias
        if (fe_extra(source1.root, FeInstStack)->item != 
            fe_extra(source2.root, FeInstStack)->item
        ) {
            return false;
        }
        // these locations will only alias each other
        // IF their access ranges overlap.
        return ranges_overlap_right_excl(
            source1.offset, source1.offset + width1, 
            source2.offset, source2.offset + width2
        );
    }

    // offsets from the same pointer
    if (source1.root == source2.root) {
        // these locations will only alias each other
        // IF their access ranges overlap.
        return ranges_overlap_right_excl(
            source1.offset, source1.offset + width1, 
            source2.offset, source2.offset + width2
        );
    }

    // TODO this is a pessimistic answer since we're not smart enough yet :(
    return true;
}


static bool immediate_uses_escape(FeInst* ptr) {
    for_n(i, 0, ptr->use_len) {
        FeInst* use = FE_USE_PTR(ptr->uses[i]);
        usize use_index = ptr->uses[i].idx;

        // figure out of this use is an escaping use
        switch (use->kind) {
        case FE_STORE:
            // the store is using it as a value
            if (use_index == 2) {
                return true;
            }
            break;
        case FE_CALL:
            // the call is using it as a parameter
            if (use_index >= 2) {
                return true;
            }
            break;
        case FE_IADD:
        case FE_ISUB:
            return immediate_uses_escape(use);
        default:
            break;
        }
    }

    return false;
}

static bool pointer_may_escape(FeFunc* f, FeInst* ptr) {
    // get 'root' pointer
    FeInst* root = trace_ptr_offset(ptr).root;

    if (root->kind == FE_STACK_ADDR) {
        return immediate_uses_escape(ptr);
    }

    return true;
}

// returns true if i1 may cause aliasing effects to i2, or vice versa.
bool fe_insts_may_alias(FeFunc* f, FeInst* i1, FeInst* i2) {
    if (i1->kind == FE__ROOT || i2->kind == FE__ROOT) {
        return true;
    }

    if (i1->kind == FE_CALL || i2->kind == FE_CALL) {
        // calls always create 'choke points' in the memory space
        // only further analysis of a function can relax this
        // (analysis which we don't currently do)
        return true;
    }

    if (i1->kind == FE_RETURN) {
        // this is the return instruction. 
        // it only aliases pointers that escape
        // (stack stores that do not escape dont affect it)

        if (!is_load_or_store(i2)) {
            return true;
        }
        
        FeInst* ptr = i2->inputs[1];
    
        return pointer_may_escape(f, ptr);
    }

    if (is_load_or_store(i1) && is_load_or_store(i2)) {
        // analyze pointers
        return pointers_may_overlap(
            i1->inputs[1], 
            load_or_store_width(i1), 
            i2->inputs[1],
            load_or_store_width(i2)
        );
    }

    // pessimistic answer :( not smart enough yet
    return true;
}
