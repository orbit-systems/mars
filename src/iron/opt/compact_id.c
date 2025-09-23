#include "common/util.h"
#include "iron/iron.h"

// compact node and block ids for smaller worklists and maps

void fe_opt_compact_ids(FeFunc* f) {
    FeInst** insts = fe_malloc(f->max_id * sizeof(insts[0]));
    FeBlock** blocks = fe_malloc(f->max_block_id * sizeof(blocks[0]));

    
    for_blocks(block, f) {
        blocks[block->id] = block;
        for_inst(inst, block) {
            insts[inst->id] = inst;
        }
    }

    usize counter = 0;
    for_n (i, 0, f->max_id) {
        FeInst* inst = insts[i];
        if (inst == nullptr) {
            continue;
        }
        inst->id = counter;
        counter += 1;
    }

    counter = 0;
    for_n (i, 0, f->max_block_id) {
        FeBlock* block = blocks[i];
        if (block == nullptr) {
            continue;
        }
        block->id = counter;
        counter += 1;
    }

    fe_free(insts);
    fe_free(blocks);
}