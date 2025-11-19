#include "common/vec.h"
#include "iron/iron.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>

// nothing aliases anything else (even itself), everything depends on root
// gives WEIRDDDDD behavior
void fe_solve_mem_root(FeFunc* f) {
    // only assume everything depends on (root)

    fprintf(stderr, "warning: use fe_solve_mem_root at your own risk\n");

    FeInst* root = f->entry_block->bookend->next;
    FE_ASSERT(root->kind == FE__ROOT);

    for_blocks(b, f) {
        for_inst(inst, b) {
            bool is_single_use = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_SINGLE_USE);
            if (is_single_use) {
                fe_set_input(f, inst, 0, root);
            }
        }
    }
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


static bool is_stack_operation(FeFunc* f, FeInst* inst) {
    switch (inst->kind) {
    case FE_STORE:
    case FE_LOAD:
        FeInst* ptr = inst->inputs[1];
        ptr = trace_ptr_offset(ptr).root;
        return ptr->kind == FE_STACK_ADDR;
    default:
        return false;
    }
}

static FeInst* def_chain_for_block(FeFunc* f, FeBlock* b, FeCompactMap* memory_effects) {
    FeInst* last_def = nullptr;
    for_inst(inst, b) {
        bool is_def = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_DEF);
        bool is_single_use = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_SINGLE_USE);
        
        if (!is_def && !is_single_use) {
            continue;
        }

        // switch (inst->kind) {
        // case FE_STORE:
        // case FE_LOAD:
        //     break;
        // default:
        //     break;
        // }

        if (is_single_use && inst->kind != FE_MEM_PHI) {
            if (last_def == nullptr) {
                // calculate def chain for predecessor if necessary
                FE_ASSERT(b->pred_len == 1);
                FeInst** last_def_ptr = (FeInst**) fe_cmap_get(memory_effects, b->pred[0]->id);
                if (last_def_ptr == nullptr) {
                    last_def = *last_def_ptr;
                } else {
                    last_def = def_chain_for_block(f, b->pred[0], memory_effects);
                }
            }
            fe_set_input(f, inst, 0, last_def);
        }

        if (is_def) {
            last_def = inst;
        }
    }

    if (last_def == nullptr) {
        // calculate def chain for predecessor if necessary
        FE_ASSERT(b->pred_len == 1);
        FeInst** last_def_ptr = (FeInst**) fe_cmap_get(memory_effects, b->pred[0]->id);
        if (last_def_ptr == nullptr) {
            last_def = *last_def_ptr;
        } else {
            last_def = def_chain_for_block(f, b->pred[0], memory_effects);
        }
    }

    fe_cmap_put(memory_effects, b->id, (uintptr_t)last_def);

    return last_def;
}

// under the assumption that everything aliases everything else
void fe_solve_mem_pessimistic(FeFunc* f) {

    // FE_ASSERT(f->entry_block == f->last_block);

    FeCompactMap memory_effects;
    fe_cmap_init(&memory_effects);

    for_blocks(block, f) {
        if (block->pred_len > 1) {
            // add a memory phi if there might be multiple merging memory effects
            FeInst* mphi = fe_inst_mem_phi(f, block->pred_len);
            fe_append_begin(block, mphi);
        }
    }

    for_blocks(block, f) {
        uintptr_t* last_def_ptr = fe_cmap_get(&memory_effects, block->id);
        FeInst* last_def;
        if (last_def_ptr == nullptr) {
            // calculate last def
            last_def = def_chain_for_block(f, block, &memory_effects);
        } else {
            last_def = *(FeInst**) last_def_ptr;
        }
    }

    for_blocks(block, f) {
        FeInst* first_inst = block->bookend->next;
        if (first_inst->kind == FE_MEM_PHI) {
            for_n(i, 0, block->pred_len) {
                FeBlock* src_block = block->pred[i];
                FeInst* src_value = *(FeInst**)fe_cmap_get(&memory_effects, src_block->id);
                fe_phi_add_src(f, first_inst, src_value, src_block);
            }
        }
    }
    fe_cmap_destroy(&memory_effects);
}

// quick digression

void fe_amap_init(FeAliasMap* amap) {
    fe_cmap_init(&amap->cmap);
    amap->num_cat = 0;
}

void fe_amap_destroy(FeAliasMap* amap) {
    fe_cmap_destroy(&amap->cmap);
    *amap = (FeAliasMap){};
}

FeAliasCategory fe_amap_new_category(FeAliasMap* amap) {
    return (FeAliasCategory){amap->num_cat++};
}

void fe_amap_add(FeAliasMap* amap, u32 inst_id, FeAliasCategory cat) {
    uintptr_t* handle = fe_cmap_get(&amap->cmap, inst_id);
    if (handle == nullptr) {
        fe_cmap_put(&amap->cmap, inst_id, (cat._ << 1) | 1);
        return;
    }

    if (*handle & 1) {
        if ((*handle >> 1) == cat._) {
            return;
        }

        // this is a single handle. we need to create a vec to hold it and the next one
        Vec(FeAliasCategory) handles = vec_new(FeAliasCategory, 4);
        vec_append(&handles, (FeAliasCategory){*handle >> 1});
        vec_append(&handles, cat);
        *handle = (uintptr_t) handles;
    } else {
        // this is multiple handles. just add it to the vec if not already there
        Vec(FeAliasCategory) handles = (Vec(FeAliasCategory)) *handle;
        for_n(i, 0, vec_len(handles)) {
            if (handles[i]._ == cat._) {
                return;
            }
        }
        vec_append(&handles, cat);
    }
}

usize fe_amap_get_len(FeAliasMap* amap, u32 inst_id) {
    uintptr_t* handle = fe_cmap_get(&amap->cmap, inst_id);
    if (handle == nullptr) {
        return 0;
    }
    if (*handle & 1) {
        return 1;
    } else {
        Vec(FeAliasCategory) handles = (Vec(FeAliasCategory)) *handle;
        return vec_len(handles);
    }
}

FeAliasCategory* fe_amap_get(FeAliasMap* amap, u32 inst_id) {
    uintptr_t* handle = fe_cmap_get(&amap->cmap, inst_id);
    if (handle == nullptr) {
        return nullptr;
    }
    if (*handle & 1) {
        // terrible lmao
        thread_local static FeAliasCategory thing;
        thing = (FeAliasCategory){*handle >> 1};
        return &thing;
    } else {
        Vec(FeAliasCategory) handles = (Vec(FeAliasCategory)) *handle;
        return handles;
    }
}

// back to your regularly-scheduled programming

static void recursive_solve(FeFunc* f, FeBlock* b, FeBlock* pred, FeAliasMap* amap, FeInst** latest_def) {
    b->flags = true; // visited

    for_inst(inst, b) {
        bool is_def = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_DEF);
        bool is_single_use = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_SINGLE_USE);
        bool is_multi_use = fe_inst_has_trait(inst->kind, FE_TRAIT_MEM_MULTI_USE);
        
        if (!is_def && !(is_single_use | is_multi_use)) {
            continue;
        }

        if (inst->kind == FE__ROOT || inst->kind == FE_RETURN) {
            for_n(i, 0, amap->num_cat) {
                fe_amap_add(amap, inst->id, (FeAliasCategory){i});
            }
        }

        usize num_aliases = fe_amap_get_len(amap, inst->id);
        FeAliasCategory* aliases = fe_amap_get(amap, inst->id);

        if (is_single_use) {
            if (num_aliases > 1) {
                // create a memory merge.
                FeInst* merge = fe_inst_mem_merge(f, num_aliases);
                for_n(i, 0, num_aliases) {
                    fe_add_input(f, merge, latest_def[aliases[i]._]);
                }
                fe_insert_before(inst, merge);
                fe_set_input(f, inst, 0, merge);
            } else {
                fe_set_input(f, inst, 0, latest_def[aliases[0]._]);
            }
        } else if (inst->kind == FE_MEM_PHI) {
            for_n(i, 0, num_aliases) {
                fe_phi_add_src(f, inst, latest_def[aliases[i]._], pred);
            }
        } else if (is_multi_use) {
            for_n(i, 0, num_aliases) {
                fe_add_input(f, inst, latest_def[aliases[i]._]);
            }
        }

        if (is_def) {
            for_n(i, 0, num_aliases) {
                latest_def[aliases[i]._] = inst;
            }
        }
    }

    for_n(i, 0, b->succ_len) {
        FeBlock* succ = b->succ[i];
        if (succ->flags) {
            // visited before! we fill out the phis and then keep going
            for_inst(phi, succ) {
                if (phi->kind != FE_MEM_PHI) {
                    continue;
                }

                FE_ASSERT(fe_amap_get_len(amap, phi->id) == 1);
                FeAliasCategory cat = fe_amap_get(amap, phi->id)[0];
                fe_phi_add_src(f, phi, latest_def[cat._], b);
            }
        } else {
            FeInst* latest_def_next[amap->num_cat];
            memcpy(latest_def_next, latest_def, sizeof(FeInst*) * amap->num_cat);

            recursive_solve(f, succ, b, amap, latest_def_next);
        }
    }
}


void fe_solve_mem(FeFunc* f, FeAliasMap* amap) {

    for_blocks(block, f) {

        block->flags = false;
        if (block->pred_len > 1) {
            for_n(cat, 0, amap->num_cat) {
                // add a memory phi if there might be multiple merging memory effects
                FeInst* mphi = fe_inst_mem_phi(f, block->pred_len);
                fe_append_begin(block, mphi);
                fe_amap_add(amap, mphi->id, (FeAliasCategory){cat});
            }
        }
    }

    FeInst* latest_def[amap->num_cat];
    for_n(i, 0, amap->num_cat) {
        latest_def[i] = nullptr;
    }

    recursive_solve(f, f->entry_block, nullptr, amap, latest_def);
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

static bool pointer_may_escape(FeInst* ptr) {
    // get 'root' pointer
    FeInst* root = trace_ptr_offset(ptr).root;

    if (root->kind == FE_STACK_ADDR) {
        return immediate_uses_escape(ptr);
    }

    return true;
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
    } else if (source1.root->kind == FE_STACK_ADDR) {
        return pointer_may_escape(source1.root);
    } else if (source2.root->kind == FE_STACK_ADDR) {
        return pointer_may_escape(source2.root);
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

static bool insts_may_alias(FeFunc* f, FeInst* i1, FeInst* i2) {
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
    
        return pointer_may_escape(ptr);
    }

    if (i1->kind == FE_MEM_PHI) {
        for_n(i, 0, i1->in_len) {
            if (fe_insts_may_alias(f, i1->inputs[i], i2)) {
                return true;
            }
        }
        return false;
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

// returns true if i1 may cause aliasing effects to i2, or vice versa.
bool fe_insts_may_alias(FeFunc* f, FeInst* i1, FeInst* i2) {
    return insts_may_alias(f, i1, i2) && insts_may_alias(f, i2, i1);
}
