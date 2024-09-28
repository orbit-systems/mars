#include "iron/iron.h"
#include "common/ptrmap.h"

/* pass "stackprom" - promote stack objects to registers

*/

PtrMap phi2obj = {0};

static bool candidate_for_stackprom(FeStackObject* obj, FeFunction* f) {
    
    // only promote if you're storing a scalar.
    if (!fe_type_is_scalar(obj->t)) {
        return false;
    }

    // only promote if the scalar is only used through stack loads/stores
    foreach(FeBasicBlock* bb, f->blocks) {
        for_fe_inst(inst, *bb) {
            if (inst->kind == FE_INST_STACK_ADDR && ((FeInstStackAddr*)inst)->object == obj) {
                return false;
            }
        }
    }
    return true;
}

static bool block_contains_store_to(FeBasicBlock* bb, FeStackObject* obj) {
    for_fe_inst(inst, *bb) {
        if (inst->kind != FE_INST_STACK_STORE) {
            continue;
        }
        FeInstStackStore* store = (FeInstStackStore*) inst;
        if (store->location == obj) {
            return true;
        }
    }
    return false;
}

static void mark_and_place_phis(FeFunction* f, FeStackObject* obj) {
    foreach(FeBasicBlock* bb, f->blocks) {
        if (!block_contains_store_to(bb, obj)) {
            continue;
        }

        // block contains a store to obj, so we have to add phis to unmarked bbs
        for_range(n, 0, bb->cfg_node->domfront_len) {

            FeCFGNode* domfront_node = bb->cfg_node->domfront[n];
            FeBasicBlock* domfront_block = domfront_node->bb;

            if (domfront_block->flags != (u64)obj) {
                FeInst* phi = fe_inst_phi(f, domfront_node->in_len, obj->t);
                fe_insert_inst_before(phi, domfront_block->start);
                domfront_block->flags = (u64)obj;
                ptrmap_put(&phi2obj, phi, obj);
            }
        }
    }
}

struct {
    FeStackObject* obj;

    struct {
        FeInst* inst;
        FeBasicBlock* block;
    }* at;
    u32 len;
    u32 cap;
} def_stack;

static FeInst* def_current_inst() {
    return def_stack.at[def_stack.len-1].inst;
}

static FeBasicBlock* def_current_block() {
    return def_stack.at[def_stack.len-1].block;
}

static void def_set_current_inst(FeInst* inst) {
    def_stack.at[def_stack.len-1].inst = inst;
}

static void def_set_current_block(FeBasicBlock* block) {
    def_stack.at[def_stack.len-1].block = block;
}

static void def_push(FeBasicBlock* block, FeInst* inst) {
    if (def_stack.len == def_stack.cap) {
        def_stack.cap *= 2;
        def_stack.at = fe_realloc(def_stack.at, def_stack.cap);
    }

    def_stack.at[def_stack.len].block = block;
    def_stack.at[def_stack.len].inst  = inst;
    def_stack.len++;
}

// push a copy of the current definition.
static void def_push_copy() {
    def_push(def_current_block(), def_current_inst());
}

static void def_pop() {
    if (def_stack.len != 0) def_stack.len--;
}

#define DFS_VISITED 12345

static void rename_defs(FeBasicBlock* block) {

    // find phi and add sources
    for (FeInstPhi* phi = (FeInstPhi*)block->start; phi->base.kind == FE_INST_PHI; phi = (FeInstPhi*)phi->base.next) {
        if (ptrmap_get(&phi2obj, phi) != def_stack.obj) {
            continue;
        }
        // get the current values
        fe_add_phi_source(block->function, phi, def_current_inst(), def_current_block());
        def_set_current_block(block);
        def_set_current_inst((FeInst*)phi);
        break;
    }

    if (block->flags == DFS_VISITED) {
        return;
    }

    block->flags = DFS_VISITED;


    for_fe_inst(inst, *block) {
        switch (inst->kind) {
        case FE_INST_STACK_LOAD:
            FeInstStackLoad* load = (FeInstStackLoad*) inst;
            if (load->location != def_stack.obj) break;

            FeInst* load_value = def_current_inst();

            fe_rewrite_uses(block->function, inst, load_value);

            fe_remove(inst);
            break;
        case FE_INST_STACK_STORE:
            FeInstStackStore* store = (FeInstStackStore*) inst;
            if (store->location != def_stack.obj) break;
            
            def_set_current_inst(store->value);
            def_set_current_block(block);
            fe_remove(inst);
            break;
        case FE_INST_JUMP:
            FeInstJump* jump = (FeInstJump*) inst;
            rename_defs(jump->dest);
            break;
        case FE_INST_BRANCH:
            FeInstBranch* branch = (FeInstBranch*) inst;
            def_push_copy();
            rename_defs(branch->if_true);
            def_pop();
        
            def_push_copy();
            rename_defs(branch->if_false);
            def_pop();
            break;
        }
    }

}

static void per_function(FeFunction* f) {

    if (def_stack.at == NULL) {
        def_stack.cap = 16;
        def_stack.at = fe_malloc(sizeof(def_stack.at[0]) * def_stack.cap);
    }

    if (phi2obj.keys == NULL) {
        ptrmap_init(&phi2obj, f->blocks.len);
    }

    // foreach(FeStackObject* obj, f->stack) {
    for_range(obji, 0, f->stack.len) {
        FeStackObject* obj = f->stack.at[obji];
        bool can_stackprom = candidate_for_stackprom(obj, f);
        if (!can_stackprom) continue;

        ptrmap_reset(&phi2obj);

        mark_and_place_phis(f, obj);

        def_stack.len = 0;
        def_stack.obj = obj;

        def_push(f->blocks.at[0], NULL);
        rename_defs(f->blocks.at[0]);

        foreach(FeBasicBlock* bb, f->blocks) {
            bb->flags = 0;
        }

        da_remove_at(&f->stack, obji);
        obji--;
    }
}

static void run_pass_stackprom(FeModule* m) {
    for_urange(i, 0, m->functions_len) {
        per_function(m->functions[i]);
    }
}

FePass fe_pass_stackprom = {
    .name = "stackprom",
    .callback = run_pass_stackprom,
    .requires_cfg = true,
};