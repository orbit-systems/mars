#include "iron/iron.h"

static void traverse_post_order(FeBlock* b, FeBlock** blocks, usize* index, bool renumber) {
    if (b->flags) {
        return;
    }

    b->flags = 1;

    for_n(i, 0, b->succ_len) {
        traverse_post_order(b->succ[i], blocks, index, renumber);
    }

    *index -= 1;
    if (renumber) {
        b->id = *index;
    }
    blocks[*index] = b;
}

void fe_write_blocks_rpo(FeFunc* f, FeBlock** output_array, bool renumber) {
    for_blocks(block, f) {
        block->flags = 0;
    }

    usize index = f->num_blocks;
    traverse_post_order(f->entry_block, output_array, &index, renumber);
    if (renumber) {
        f->next_block_id = f->num_blocks;
    }
    FE_ASSERT(index == 0);
}

void fe_opt_reorder_blocks_rpo(FeFunc* f) {
    if (f->num_blocks <= 1) {
        return;
    }
    
    FeBlock* blocks_rpo[f->num_blocks];
    fe_write_blocks_rpo(f, blocks_rpo, true);

    blocks_rpo[0]->list_next = blocks_rpo[1];
    blocks_rpo[0]->list_prev = nullptr;
    for_n(i, 1, f->next_block_id - 1) {
        blocks_rpo[i]->list_next = blocks_rpo[i + 1];
        blocks_rpo[i]->list_prev = blocks_rpo[i - 1];
    }
    blocks_rpo[f->next_block_id - 1]->list_prev = blocks_rpo[f->next_block_id - 2];
    blocks_rpo[f->next_block_id - 1]->list_next = nullptr;
}
