#include "iron/iron.h"

/* pass "cfg" - populate and provide information about control flow graphs

*/

static Arena cfg_nodes;

static u32 pre_order_index;

static void populate_pre_order(FeCFGNode* root) {
    if (root->pre_order_number != 0) return;

    root->pre_order_number = ++pre_order_index;

    for_range(i, 0, root->out_len) {
        populate_pre_order(root->outgoing[i]);
    }
}

// initialize cfg nodes
static void init_cfg_nodes(FeFunction* fn) {

    // give every basic block a CFG node
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = arena_alloc(&cfg_nodes, sizeof(FeCFGNode), alignof(FeCFGNode));
        *node = (FeCFGNode){0};
        node->bb = bb;
        bb->cfg_node = node;
    }

    // populate outgoing and prepare incoming
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;

        switch (bb->end->kind){
        case FE_INST_RETURN:
            node->out_len = 0;
            break;
        case FE_INST_JUMP:
            FeInstJump* jump = (FeInstJump*)bb->end;
            // set outgoing
            node->out_len = 1;
            node->outgoing = arena_alloc(&cfg_nodes, sizeof(FeCFGNode) * node->out_len, alignof(FeCFGNode));
            node->outgoing[0] = jump->dest->cfg_node;
            // increment incoming len of target node
            jump->dest->cfg_node->in_len++;
            break;
        case FE_INST_BRANCH:
            FeInstBranch* branch = (FeInstBranch*)bb->end;
            // set outgoing
            node->out_len = 2;
            node->outgoing = arena_alloc(&cfg_nodes, sizeof(FeCFGNode) * node->out_len, alignof(FeCFGNode));
            node->outgoing[0] = branch->if_true->cfg_node;
            node->outgoing[1] = branch->if_false->cfg_node;
            // increment incoming len of target nodes
            branch->if_true->cfg_node->in_len++;
            branch->if_false->cfg_node->in_len++;
            break;
        default:
            CRASH("INVALID TERMINATOR"); // todo: replace this with an FeMessage
            break;
        }
    }

    // allocate incoming
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;
        node->incoming = arena_alloc(&cfg_nodes, sizeof(FeCFGNode) * node->in_len, alignof(FeCFGNode));
        node->in_len = 0;
    }

    // populate incoming
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;

        switch (bb->end->kind){
        case FE_INST_RETURN:
            break;
        case FE_INST_JUMP:
            FeInstJump* jump = (FeInstJump*)bb->end;
            FeCFGNode* target = node->outgoing[0];
            target->incoming[target->in_len++] = node;
            break;
        case FE_INST_BRANCH:
            FeInstBranch* branch = (FeInstBranch*)bb->end;
            FeCFGNode* true_target = node->outgoing[0];
            FeCFGNode* false_target = node->outgoing[0];
            true_target->incoming[true_target->in_len++] = node;
            false_target->incoming[false_target->in_len++] = node;
            break;
        default:
            CRASH("INVALID TERMINATOR"); // todo: replace this with an FeMessage
            break;
        }
    }

    // populate pre-order number
    pre_order_index = 0;
    populate_pre_order(fn->blocks.at[0]->cfg_node);
}

static void per_function(FeFunction* fn) {
    init_cfg_nodes(fn);
    foreach(FeBasicBlock* bb, fn->blocks) {
        
    }
}

static void module(FeModule* m) {

    // if cfg info already exists, destroy it
    if (cfg_nodes.arena_size != 0) arena_delete(&cfg_nodes);
    cfg_nodes = arena_make(256*sizeof(FeCFGNode));
    
    for_range(i, 0, m->functions_len) {
        per_function(m->functions[i]);
    }
}

FePass fe_pass_cfg = {
    .name = "cfg",
    .callback = module,
};