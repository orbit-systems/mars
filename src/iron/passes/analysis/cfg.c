#include "iron/iron.h"

/* pass "cfg" - populate and provide information about control flow graphs

    uses the Lengauer-Tarjan dominator tree algorithm
    https://www.cs.princeton.edu/courses/archive/fall03/cs528/handouts/a%20fast%20algorithm%20for%20finding.pdf
*/

// information for Lengauer-Tarjan
// shorthand: vi means v.pre_order_index
static struct {
    // parent[vi] = the parent of vertex v in the dfs spanning tree
    FeCFGNode** parent;

    // semidom[vi] = index of computed semi-dominator of v
    //      before v is numbered, semidom[vi] = 0
    //      after v is numbered before the semidominator is computed, semidom[vi] = vi
    //      after the semidominator of v is computed, semidom[vi] = semidominator of v
    u32* semidom;

    // vertex[vi] = v
    FeCFGNode** vertex;

    // forest_parent[vi] = w
    //      after lt_link(w, v)
    FeCFGNode** forest_parent;


    // lt_get_bucket(v) = set of vertices whose semidominator is v
    FeCFGNode** bucket;
    u32* bucket_lens;

    // number of nodes + 1
    u32 array_len;
} LT;

static u32 pre_order_index;
static void populate_pre_order(FeCFGNode* v, FeCFGNode* parent) {
    v->pre_order_index = ++pre_order_index;

    LT.vertex [v->pre_order_index] = v;
    LT.parent [v->pre_order_index] = parent;
    LT.semidom[v->pre_order_index] = v->pre_order_index;

    for_range(i, 0, v->out_len) {
        FeCFGNode* child = v->outgoing[i];
        if (child->pre_order_index == 0) {
            populate_pre_order(child, v);
        }
    }
}

// add edge (parent -> child) to forest
static void lt_link(FeCFGNode* parent, FeCFGNode* child) {
    LT.forest_parent[child->pre_order_index] = parent;
}

// if is_root(v) return v;
// return u where !is_root(v) of minimum semidom(ui) on the path r -> v
static FeCFGNode* lt_eval(FeCFGNode* v) {
    // if is_root(v) return v;
    if (LT.forest_parent[v->pre_order_index] == NULL) return v;

    // return u where !is_root(v) of minimum semidom(ui) on the path r -> v
    FeCFGNode* u = v;
    for (FeCFGNode* candidate = u; LT.forest_parent[candidate->pre_order_index] != NULL; candidate = LT.forest_parent[candidate->pre_order_index]) {
        if (LT.semidom[candidate->pre_order_index] < LT.semidom[u->pre_order_index]) {
            u = candidate;
        }
    }

    return u;
}

static void lt_init(FeFunction* fn) {  
    LT.array_len     = fn->blocks.len + 1;  
    LT.parent        = fe_malloc(sizeof(LT.parent[0]) * LT.array_len);
    LT.semidom       = fe_malloc(sizeof(LT.semidom[0]) * LT.array_len);
    LT.vertex        = fe_malloc(sizeof(LT.vertex[0]) * LT.array_len);
    LT.forest_parent = fe_malloc(sizeof(LT.forest_parent[0]) * LT.array_len);
    LT.bucket        = fe_malloc(sizeof(LT.bucket[0]) * LT.array_len * LT.array_len);
    LT.bucket_lens   = fe_malloc(sizeof(LT.bucket_lens[0]) * LT.array_len);
}

static forceinline FeCFGNode** lt_get_bucket(FeCFGNode* w) {
    return &LT.bucket[(w->pre_order_index - 1) * LT.array_len];
}

static forceinline FeCFGNode** lt_get_bucket_from_index(u32 pre_order_index) {
    return &LT.bucket[(pre_order_index - 1) * LT.array_len];
}

static forceinline void lt_add_to_bucket(FeCFGNode* w, FeCFGNode* node) {
    FeCFGNode** bucket = lt_get_bucket(w);
    bucket[LT.bucket_lens[w->pre_order_index]++] = node;
}

static forceinline void lt_add_to_bucket_from_index(u32 pre_order_index, FeCFGNode* node) {
    FeCFGNode** bucket = lt_get_bucket_from_index(pre_order_index);
    bucket[LT.bucket_lens[pre_order_index]++] = node;
}

static forceinline void lt_clear_bucket(FeCFGNode* w) {
    LT.bucket_lens[w->pre_order_index] = 0;
}

static void lt_deinit(FeFunction* fn) {
    fe_free(LT.parent);
    fe_free(LT.semidom);
    fe_free(LT.vertex);
    fe_free(LT.forest_parent);
    fe_free(LT.bucket);
    fe_free(LT.bucket_lens);
}

// initialize cfg nodes
static void init_cfg_nodes(FeFunction* fn) {

    // initialize Lengauer-Tarjan info arrays
    lt_init(fn);

    // give every basic block a CFG node
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = arena_alloc(&fn->cfg, sizeof(FeCFGNode), alignof(FeCFGNode));
        *node = (FeCFGNode){0};
        node->bb = bb;
        bb->cfg_node = node;
        node->pre_order_index = 0;
    }

    // populate outgoing and prepare incoming
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;

        switch (bb->end->kind){
        case FE_IR_RETURN:
            node->out_len = 0;
            break;
        case FE_IR_JUMP:
            FeIrJump* jump = (FeIrJump*)bb->end;
            // set outgoing
            node->out_len = 1;
            node->outgoing = arena_alloc(&fn->cfg, sizeof(FeCFGNode) * node->out_len, alignof(FeCFGNode));
            node->outgoing[0] = jump->dest->cfg_node;
            // increment incoming len of target node
            jump->dest->cfg_node->in_len++;
            break;
        case FE_IR_BRANCH:
            FeIrBranch* branch = (FeIrBranch*)bb->end;
            // set outgoing
            node->out_len = 2;
            node->outgoing = arena_alloc(&fn->cfg, sizeof(FeCFGNode) * node->out_len, alignof(FeCFGNode));
            node->outgoing[0] = branch->if_true->cfg_node;
            node->outgoing[1] = branch->if_false->cfg_node;
            // increment incoming len of target nodes
            branch->if_true->cfg_node->in_len++;
            branch->if_false->cfg_node->in_len++;
            break;
        default:
            CRASH("invalid terminator"); // todo: replace this with an FeReport
            break;
        }
    }

    // allocate incoming
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;
        node->incoming = arena_alloc(&fn->cfg, sizeof(FeCFGNode) * node->in_len, alignof(FeCFGNode));
        node->in_len = 0;
    }

    // populate incoming
    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;

        switch (bb->end->kind){
        case FE_IR_RETURN:
            break;
        case FE_IR_JUMP:
            FeIrJump* jump = (FeIrJump*)bb->end;
            FeCFGNode* jump_target = node->outgoing[0];
            jump_target->incoming[jump_target->in_len++] = node;
            break;
        case FE_IR_BRANCH:
            FeIrBranch* branch = (FeIrBranch*)bb->end;
            FeCFGNode* true_target = node->outgoing[0];
            FeCFGNode* false_target = node->outgoing[1];
            true_target->incoming[true_target->in_len++] = node;
            false_target->incoming[false_target->in_len++] = node;
            break;
        default:
            CRASH("invalid terminator"); // todo: replace this with an FeReport
            break;
        }
    }

    // LT step 1
    pre_order_index = 0;
    populate_pre_order(fn->blocks.at[0]->cfg_node, NULL);

    for (u32 i = LT.array_len - 1; i >= 2; i--) {
        FeCFGNode* w = LT.vertex[i];
        // step 2
        for_range(vi, 0, w->in_len) {
            FeCFGNode* v = w->incoming[vi];
            FeCFGNode* u = lt_eval(v);
            if (LT.semidom[u->pre_order_index] < LT.semidom[w->pre_order_index]) {
                LT.semidom[w->pre_order_index] = LT.semidom[u->pre_order_index];
            }
            lt_add_to_bucket_from_index(LT.semidom[w->pre_order_index], w);
            lt_link(LT.parent[w->pre_order_index], w);
        }
        // step 3
        for_range(vi, 0, LT.bucket_lens[LT.parent[w->pre_order_index]->pre_order_index]) {
            FeCFGNode* v = lt_get_bucket(LT.parent[w->pre_order_index])[vi];
            FeCFGNode* u = lt_eval(v);
            
            if (LT.semidom[u->pre_order_index] < LT.semidom[v->pre_order_index]) {
                v->immediate_dominator = u;
            } else {
                v->immediate_dominator = LT.parent[w->pre_order_index];
            }
        }
    }

    // step 4
    for_range(i, 2, LT.array_len - 1) {
        FeCFGNode* w = LT.vertex[i];
        if (w->immediate_dominator != LT.vertex[LT.semidom[w->pre_order_index]]) {
            w->immediate_dominator = w->immediate_dominator->immediate_dominator;
        }
    }
    fn->blocks.at[0]->cfg_node->immediate_dominator = NULL;
}

static void emit_cfg_dot(FeFunction* fn) {
    printf("--------\n");
    printf("digraph CFG {\n");

    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;
        for_range(i, 0, node->out_len) {
            printf("  \""str_fmt"\" -> \""str_fmt"\"\n", str_arg(bb->name), str_arg(node->outgoing[i]->bb->name));
        }
    }

    printf("}\n");
}

static void emit_domtree_dot(FeFunction* fn) {
    printf("--------\n");
    printf("digraph DominanceTree {\n");

    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;
        if (node->immediate_dominator != NULL) {
            printf("  \""str_fmt"\" -> \""str_fmt"\"\n", str_arg(node->immediate_dominator->bb->name), str_arg(bb->name));
        }
    }

    printf("}\n");
}

// is N strictly dominated by D?
static bool sdom(FeCFGNode* D, FeCFGNode* N) {
    // if (N == D) return false;
    while (N->immediate_dominator != NULL) {
        if (N->immediate_dominator == D) return true;
        N = N->immediate_dominator;
    }
    return false;
}

static bool dom(FeCFGNode* D, FeCFGNode* N) {
    if (N == D) return true;
    while (N->immediate_dominator != NULL) {
        if (N->immediate_dominator == D) return true;
        N = N->immediate_dominator;
    }
    return false;
}

static void compute_domfront(FeFunction* fn, FeCFGNode* N) {
    // domfront(node N) = set of all blocks that are immediate successors to blocks dominated by N, but not dominated by N themselves
    N->domfront = arena_alloc(&fn->cfg, sizeof(FeCFGNode*) * fn->blocks.len, alignof(FeCFGNode*));
    N->domfront_len = 0;

    foreach(FeBasicBlock* bb, fn->blocks) {
        FeCFGNode* node = bb->cfg_node;

        if (!dom(N, node)) continue;
        
        for_range(i, 0, node->out_len) {
            FeCFGNode* successor = node->outgoing[i];
            if (!sdom(N, successor)) {
                // add current node to domfront
                N->domfront[N->domfront_len++] = successor;
                break;
            }
        }
    }
}

static void function_cfg(FeFunction* fn) {
    if (fn->cfg_up_to_date) return;

    if (fn->cfg.list.at != NULL) arena_delete(&fn->cfg);
    fn->cfg = arena_make(sizeof(FeCFGNode)*(fn->blocks.len) + 10000);

    init_cfg_nodes(fn);

    lt_deinit(fn);

    foreach(FeBasicBlock* bb, fn->blocks) {
        compute_domfront(fn, bb->cfg_node);
    }

    fn->cfg_up_to_date = true;
    // emit_cfg_dot(fn);
    // emit_domtree_dot(fn);
}

static void module_cfg(FeModule* m) {
    for_range(i, 0, m->functions_len) {
        FeFunction* fn = m->functions[i];
        function_cfg(fn);
    }
}

FePass fe_pass_cfg = {
    .name = "cfg",
    .module = module_cfg,
    .function = function_cfg,
};