#include "deimos.h"

/* pass "cfg" - populate and provide information about control flow graphs.

*/

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
            if (!target->incoming) {
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
}

IR_Module* ir_pass_cfg(IR_Module* mod) {

    // TODO: only update the CFG for functions where it has been modified
    FOR_URANGE(i, 0, mod->functions_len) {
        pass_cfg_func(mod->functions[i]);
    }

    return mod;
}