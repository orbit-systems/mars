#include "iron/iron.h"
#include <stdio.h>

// calculate dominator tree using iterative algorithm described in
// https://www.cs.tufts.edu/~nr/cs257/archive/keith-cooper/dom14.pdf

#include <stdio.h>

// assumes pre-order number is in flags
static FeBlock* intersect(FeBlock* b1, FeBlock* b2) {
    while (b1->flags != b2->flags) {
        while (b1->flags < b2->flags) {
            b1 = b1->idom;
        }
        while (b1->flags < b2->flags) {
            b2 = b2->idom;
        }
    }
    return b1;
}

void fe_construct_domtree(FeFunc* f) {

    FeBlock* blocks_rpo[f->num_blocks];
    fe_write_blocks_rpo(f, blocks_rpo, true);

    for_n(i, 0, f->num_blocks) {
        // i think this makes sense ????????
        blocks_rpo[i]->flags = f->num_blocks - 1 - i;
    }

    FeBlock* start_node = f->entry_block;

    for_n(i, 0, f->num_blocks) {
        blocks_rpo[i]->idom = nullptr;
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for_n(i, 1, f->num_blocks) {
            FeBlock* b = blocks_rpo[i];
            FeBlock* new_idom = b->pred[0];
            for_n(j, 1, b->pred_len) {
                FeBlock* p = b->pred[j];
                if (p->idom != nullptr) {
                    new_idom = intersect(p, new_idom);
                }
            }
            if (b->idom != new_idom) {
                b->idom = new_idom;
                changed = true;
            }
        }
    }

    printf("digraph {\n");
    printf("  subgraph cfg {\n");
    for_n(i, 0, f->num_blocks) {
        FeBlock* b = blocks_rpo[i];
        for_n(j, 0, b->succ_len) {
            FeBlock* succ = b->succ[j];
            printf("    b%d -> b%d\n", b->id, succ->id);
        }
    }
    printf("  }\n");

    printf("  subgraph domtree {\n");
    printf("    edge [style=\"dashed\"]\n");
    printf("    edge [color=blue]\n");
    for_n(i, 1, f->num_blocks) {
        FeBlock* b = blocks_rpo[i];
        printf("    b%d -> b%d \n", b->id, b->idom->id);
    }
    printf("  }\n");
    printf("}\n");
}
