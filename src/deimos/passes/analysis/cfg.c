#include "deimos.h"

/* pass "cfg" - populate and provide information about control flow graphs.

*/

#define MARKED 0xDEADBEEF

static IR_BasicBlock** compute_dominator_set(IR_Function* f, IR_BasicBlock* bb, IR_BasicBlock* entry);
static void recursive_reparent(IR_BasicBlock* bb);

static void pass_cfg_func(IR_Function* f) {

    // populate forward connections and increment incoming data
    FOR_URANGE(i, 0, f->blocks.len) {
        IR_BasicBlock* bb = f->blocks.at[i];
        IR* terminator = bb->at[bb->len-1];
        
        if (bb->out_len) {
            free(bb->outgoing);
        }

        switch (terminator->tag) {
        case IR_RETURN:
            bb->out_len = 0;
            break;
        case IR_JUMP:
            bb->out_len = 1;
            IR_Jump* jump = (IR_Jump*) terminator;
            
            bb->outgoing = malloc(sizeof(bb->outgoing[0])*bb->out_len);
            bb->outgoing[0] = jump->dest;
            jump->dest->in_len++;
            break;
        case IR_BRANCH:
            bb->out_len = 2;
            IR_Branch* branch = (IR_Branch*) terminator;

            bb->outgoing = malloc(sizeof(bb->outgoing[0])*bb->out_len);
            bb->outgoing[0] = branch->if_false;
            bb->outgoing[1] = branch->if_true;
            branch->if_false->in_len++;
            branch->if_false->out_len++;
            break;
        default:
            CRASH("invalid basic block terminator instruction");
            break;
        }

        if (bb->incoming) free(bb->incoming);
        bb->incoming = NULL;
    }

    // populate backward connections
    FOR_URANGE(i, 0, f->blocks.len) {
        IR_BasicBlock* bb = f->blocks.at[i];

        FOR_URANGE(conn, 0, bb->out_len) {
            IR_BasicBlock* target = bb->outgoing[conn];
            if (target->incoming != NULL) {
                target->incoming = malloc(sizeof(IR_BasicBlock*) * bb->in_len);
                memset(target->incoming, 0, sizeof(IR_BasicBlock*) * bb->in_len);
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
    FOR_URANGE(i, 0, f->blocks.len) {
        f->blocks.at[i]->domset = compute_dominator_set(f, f->blocks.at[i], f->blocks.at[f->entry_idx]);
    }
    
    // starting at the dominator set for the entry, recursively re-parent idom to construct dominator tree
    // this should work ???? permission to slap me if it doesnt
    recursive_reparent(f->blocks.at[f->entry_idx]);
}

static void recursive_reparent(IR_BasicBlock* bb) {
    for (size_t i = 0; bb->domset[i] != NULL; i++) {
        if (bb->domset[i] == bb) continue;
        bb->domset[i]->idom = bb;
    }

    for (size_t i = 0; bb->domset[i] != NULL; i++) {
        if (bb->domset[i] == bb) continue;
        recursive_reparent(bb->domset[i]);
    }
}

static void recursive_mark(IR_BasicBlock* bb, IR_BasicBlock* avoid, u64 mark) {
    if (bb == avoid || bb->flags == mark) return;
    
    bb->flags = mark; // mark

    FOR_RANGE(i, 0, bb->out_len) {
        recursive_mark(bb->outgoing[i], avoid, mark);
    }
}

// returns a null-terminated, list of blocks that are dominated by the input basic block
// naive quadratic algorithm
static IR_BasicBlock** compute_dominator_set(IR_Function* f, IR_BasicBlock* bb, IR_BasicBlock* entry) {

    // reset visited flags
    FOR_URANGE(i, 0, f->blocks.len) {
        f->blocks.at[i]->flags = 0;
    }
    
    recursive_mark(entry, bb, MARKED);

    // find blocks that are not marked
    u64 num_dominated = 0;
    FOR_URANGE(i, 0, f->blocks.len) {
        if (f->blocks.at[i]->flags != MARKED) {
            num_dominated++;
        }
    }

    IR_BasicBlock** domset = malloc(sizeof(IR_BasicBlock*) * (num_dominated + 1));

    u64 next = 0;
    FOR_URANGE(i, 0, f->blocks.len) {
        if (f->blocks.at[i]->flags != MARKED) {
            domset[next++] = f->blocks.at[i];
        }
    }
    domset[num_dominated] = NULL;
    return domset;
}

IR_Module* ir_pass_cfg(IR_Module* mod) {

    // TODO: only update the CFG for functions where it has been modified
    FOR_URANGE(i, 0, mod->functions_len) {
        pass_cfg_func(mod->functions[i]);
    }

    return mod;
}