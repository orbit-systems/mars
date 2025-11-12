#include "common/util.h"
#include "iron/iron.h"

// compact node and block ids for smaller worklists and maps

void fe_opt_compact_ids(FeFunc* f) {
    FeInst** insts = fe_zalloc(f->id_count * sizeof(insts[0]));
    FeBlock** blocks = fe_zalloc(f->block_count * sizeof(blocks[0]));

    for_blocks(block, f) {
        blocks[block->id] = block;
        insts[block->bookend->id] = block->bookend;
        for_inst(inst, block) {
            insts[inst->id] = inst;
        }
    }

    usize counter = 0;
    for_n (i, 0, f->id_count) {
        FeInst* inst = insts[i];
        if (inst == nullptr) {
            continue;
        }
        inst->id = counter;
        counter += 1;
    }

    f->id_count = counter;

    counter = 0;
    for_n (i, 0, f->block_count) {
        FeBlock* block = blocks[i];
        if (block == nullptr) {
            continue;
        }
        block->id = counter;
        counter += 1;
    }

    f->block_count = counter;

    fe_free(insts);
    fe_free(blocks);
}
