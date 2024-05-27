#include "atlas.h"

/* pass "cfg" - populate and provide information about control flow graphs.

*/

#define MARKED 0xDEADBEEF

// big fuck-off function header lol
static AIR_BasicBlock** compute_dominator_set(AIR_Function* f, AIR_BasicBlock* bb, AIR_BasicBlock* entry, u32* domset_len);

static void recursive_reparent(AIR_BasicBlock* bb);

static void pass_cfg_func(AIR_Function* f) {

    // populate forward connections and increment incoming data
    for_urange(i, 0, f->blocks.len) {
        AIR_BasicBlock* bb = f->blocks.at[i];
        AIR* terminator = bb->at[bb->len-1];
        
        if (bb->out_len) {
            mars_free(bb->outgoing);
        }

        switch (terminator->tag) {
        case AIR_RETURN:
            bb->out_len = 0;
            break;
        case AIR_JUMP:
            bb->out_len = 1;
            AIR_Jump* jump = (AIR_Jump*) terminator;
            
            bb->outgoing = mars_alloc(sizeof(bb->outgoing[0])*bb->out_len);
            bb->outgoing[0] = jump->dest;
            jump->dest->in_len++;
            break;
        case AIR_BRANCH:
            bb->out_len = 2;
            AIR_Branch* branch = (AIR_Branch*) terminator;

            bb->outgoing = mars_alloc(sizeof(bb->outgoing[0])*bb->out_len);
            bb->outgoing[0] = branch->if_false;
            bb->outgoing[1] = branch->if_true;
            branch->if_false->in_len++;
            branch->if_false->out_len++;
            break;
        default:
            CRASH("invalid basic block terminator instruction");
            break;
        }

        if (bb->incoming) mars_free(bb->incoming);
        bb->incoming = NULL;
    }

    // populate backward connections
    for_urange(i, 0, f->blocks.len) {
        AIR_BasicBlock* bb = f->blocks.at[i];

        for_urange(conn, 0, bb->out_len) {
            AIR_BasicBlock* target = bb->outgoing[conn];
            if (target->incoming != NULL) {
                target->incoming = mars_alloc(sizeof(AIR_BasicBlock*) * bb->in_len);
            }

            for (uintptr_t j = 0; j < bb->in_len; j++) {
                if (target->incoming[j] == NULL) {
                    target->incoming[j] = bb;
                    break;
                }
            }
        }
    }

    // compute dominators
    // 2d array of pointers [block][dominator]
    for_urange(i, 0, f->blocks.len) {
        
        f->blocks.at[i]->domset = compute_dominator_set(f, f->blocks.at[i], f->blocks.at[f->entry_idx], &f->blocks.at[i]->domset_len);
    }
    
    // starting at the dominator set for the entry, recursively re-parent idom to construct dominator tree
    // this should work ???? permission to slap me if it doesnt
    recursive_reparent(f->blocks.at[f->entry_idx]);
}

static void recursive_reparent(AIR_BasicBlock* bb) {
    for (size_t i = 0; bb->domset[i] != NULL; i++) {
        if (bb->domset[i] == bb) continue;
        bb->domset[i]->idom = bb;
    }

    for (size_t i = 0; bb->domset[i] != NULL; i++) {
        if (bb->domset[i] == bb) continue;
        recursive_reparent(bb->domset[i]);
    }
}

static void recursive_mark(AIR_BasicBlock* bb, AIR_BasicBlock* avoid, u64 mark) {
    if (bb == avoid || bb->flags == mark) return;
    
    bb->flags = mark; // mark

    for_range(i, 0, bb->out_len) {
        recursive_mark(bb->outgoing[i], avoid, mark);
    }
}

// returns a null-terminated, list of blocks that are dominated by the input basic block
// naive quadratic algorithm
static AIR_BasicBlock** compute_dominator_set(AIR_Function* f, AIR_BasicBlock* bb, AIR_BasicBlock* entry, u32* domset_len) {

    // reset visited flags
    for_urange(i, 0, f->blocks.len) {
        f->blocks.at[i]->flags = 0;
    }
    
    recursive_mark(entry, bb, MARKED);

    // find blocks that are not marked
    u64 num_dominated = 0;
    for_urange(i, 0, f->blocks.len) {
        if (f->blocks.at[i]->flags != MARKED) {
            num_dominated++;
        }
    }

    AIR_BasicBlock** domset = mars_alloc(sizeof(AIR_BasicBlock*) * (num_dominated));

    u64 next = 0;
    for_urange(i, 0, f->blocks.len) {
        if (f->blocks.at[i]->flags != MARKED) {
            domset[next++] = f->blocks.at[i];
        }
    }
    *domset_len = num_dominated;
    return domset;
}

AIR_Module* air_pass_cfg(AIR_Module* mod) {

    // TODO: only update the CFG for functions where it has been modified
    for_urange(i, 0, mod->functions_len) {
        pass_cfg_func(mod->functions[i]);
    }

    return mod;
}