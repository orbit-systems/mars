#include "common/portability.h"
#include "common/util.h"
#include "common/vec.h"
#include <iron/iron.h>
#include <stdint.h>

static u32 hashes[256];

typedef struct {
    FeInst* inst;
    u32 hash;
} ValueNumber;

static u32 retrieve_hash(FeInst* inst, Vec(ValueNumber) vn) {
    for_n(i, 0, vec_len(vn)) {
        if (vn[i].inst == inst) {
            return vn[i].hash;
        }
    }

    // for some cases like phi inputs, we dont have a value number for it at this point.
    // fall back to the instruction pointer bits. yeah yeah whatever
    return (u32)(uintptr_t)inst;
}

static FeInst* retrieve_inst(u32 hash, Vec(ValueNumber) vn) {
    for_n(i, 0, vec_len(vn)) {
        if (vn[i].hash == hash) {
            return vn[i].inst;
        }
    }

    // unable to find hash in table
    return nullptr;
}

static u32 calculate_hash(FeInst* inst, Vec(ValueNumber) vn) {
    FE_ASSERT(inst->kind < 128);
    u32 hash = hashes[inst->kind];

    switch (inst->kind) {
    case FE_PROJ:
        hash ^= fe_extra(inst, FeInstProj)->index;
        break;
    case FE_CONST:
        hash ^= fe_extra(inst, FeInstConst)->val;
        break;
    case FE_STACK_ADDR:
        hash ^= (uintptr_t)fe_extra(inst, FeInstStack)->item;
        break;
    case FE_SYM_ADDR:
        hash ^= (uintptr_t)fe_extra(inst, FeInstSymAddr)->sym;
        break;
    case FE_STORE:
    case FE_LOAD:
        hash ^= fe_extra(inst, FeInstMemop)->align;
        hash ^= fe_extra(inst, FeInstMemop)->offset;
        break;
    case FE_BRANCH:
        hash ^= (uintptr_t)fe_extra(inst, FeInstBranch)->if_true;
        hash ^= (uintptr_t)fe_extra(inst, FeInstBranch)->if_false;
        break;
    case FE_JUMP:
        hash ^= (uintptr_t)fe_extra(inst, FeInstJump)->to;
        break;
    case FE_PHI:
        for_n(i, 0, inst->in_len) {
            hash ^= (uintptr_t)fe_extra(inst, FeInstPhi)->blocks[i];
        }
        break;
    }

    for_n(i, 0, inst->in_len) {
        FeInst* input = inst->inputs[i];
        hash ^= retrieve_hash(input, vn) << (i + 1);
        // hash ^= (uintptr_t)input;
    }

    return hash;
}

// returns true if we made modifications to the program
static bool dvnt_gvn(FeFunc* f, FeBlock* b, Vec(FeBlock*)* domtree_children, Vec(ValueNumber)* vn) {
    usize vn_len = vec_len(*vn);    

    bool changed = false;

    for_inst(i, b) {
        u32 hash = calculate_hash(i, *vn);

        FeInst* retrieved = retrieve_inst(hash, *vn);
        if (retrieved != nullptr) {
            // inst is identical to some earlier instruction, so we can replace its uses
            fe_replace_uses(f, i, retrieved);
            changed = true;
        } else {
            // inst is unique, so we push it
            ValueNumber n = {
                .inst = i,
                .hash = hash,
            };
            vec_append(vn, n);
        }
    }

    Vec(FeBlock*) dom_children = domtree_children[b->id];
    if (dom_children != nullptr) {
        for_n(i, 0, vec_len(dom_children)) {
            changed |= dvnt_gvn(f, dom_children[i], domtree_children, vn);
        }
    }

    vec_len(*vn) = vn_len;

    return changed;
}

bool fe_opt_gvn(FeFunc* f) {
    fe_construct_domtree(f);

    Vec(FeBlock*) domtree_children[f->next_block_id];
    memset(domtree_children, 0, sizeof(domtree_children[0]) * f->next_block_id);
    
    for_blocks(b, f) {
        FeBlock* idom = b->idom;
        if (idom == nullptr) {
            continue;
        }

        if (domtree_children[idom->id] == nullptr) {
            domtree_children[idom->id] = vec_new(FeBlock*, 16);
        }

        vec_append(&domtree_children[idom->id], b);
    }

    Vec(ValueNumber) vn = vec_new(ValueNumber, 512);

    return dvnt_gvn(f, f->entry_block, domtree_children, &vn);
}


static u32 hashes[256] = {
    0x874665b3, 0xdf820ac5, 0x7b38d898, 0x6646b890, 
    0x4a564249, 0x20794b82, 0xe80b1b51, 0xf9669024, 
    0x06cdc1db, 0xda953e47, 0xa79fd1c9, 0x22ada5e1, 
    0x0dce193c, 0x0cafb3ab, 0x33de408c, 0xa343bb57, 
    0x8f1ca5e8, 0xb1685bde, 0xf4e6359c, 0x1d2f5f77, 
    0x75cce2a5, 0xdd628dd9, 0x31cf4d6f, 0xa88a18de, 
    0x1e7ab4c8, 0x03a14a77, 0xf8811ecd, 0xe3e78cc5, 
    0xee34ec15, 0xb295754f, 0x9a84fc41, 0xf3b17617, 
    0x586fbf08, 0x70c058b2, 0x8a4f4fb2, 0xa4bafe36, 
    0xba74183e, 0xe734410b, 0xc7a3942f, 0xf194f8de, 
    0xab871cfb, 0xb1d92d3d, 0x2950179b, 0x12217d6a, 
    0x61ec980c, 0x941902e7, 0x4b3c8402, 0x5486efa3, 
    0x9edbb749, 0x67e5ac1c, 0x08064a8e, 0xef51b061, 
    0x1f1ef685, 0x74610b66, 0xb0b4847e, 0xdd257ca2, 
    0xc57d9555, 0x0424cd93, 0xee206620, 0xb9bb8915, 
    0xd3ee060e, 0x8ba04a8f, 0xa440045b, 0x293c87cf, 
    0x4a7dc37f, 0xdfaa60b7, 0x4770c3c5, 0x421401bc, 
    0x8818df20, 0xab2af685, 0x5dca035a, 0xc3718cf9, 
    0xcac344ab, 0xec75c0d7, 0x7e0f2329, 0x6274d597, 
    0x340df5cf, 0x70e9716b, 0x717cb67b, 0x509bc5e9, 
    0x63ae9233, 0xbd6b9363, 0x3bb0508e, 0xb0549ac8, 
    0x6601c073, 0x839200e6, 0x3ef09fc1, 0xb3ea8f3b, 
    0x8410a869, 0x103c4cb5, 0x8f707fc0, 0xb3107bd1, 
    0xd9903882, 0x6f4f97bf, 0xf2e0cb7e, 0xb0c3dbf0, 
    0x390bd59e, 0xbab31664, 0x77a1b102, 0xced7598a, 
    0xc4c84907, 0xf39f5759, 0x33ba416a, 0x7d10b4c3, 
    0xd30c421f, 0x138e8444, 0xadbce605, 0x8b257e6d, 
    0x2ec2b3ba, 0xef2372d3, 0x7a2c4df2, 0xd178d5f3, 
    0x493ae0ea, 0xcac3ca23, 0x5f09f53e, 0xa9b9e680, 
    0xb8c2b66f, 0x4cf74f84, 0x9e8a89d7, 0xa33b546c, 
    0xbc6833ab, 0xa7ee7e3b, 0x6898cba5, 0xf457e434, 
    0x9e7adffa, 0xd1a3627d, 0x411ce190, 0xced397aa, 
    0x8eb4d2dd, 0xba3c12fc, 0xdb59a244, 0x2e9386f9, 
    0x4017c635, 0x608c944a, 0x1ac5561e, 0x2b9fe4f4, 
    0x9e34433a, 0xc9645e67, 0x946702c1, 0x5b94bb75, 
    0xf6dfa44b, 0xd6414e7b, 0xccd8f427, 0x5da28a2b, 
    0x9053efa2, 0x1cbdfd6e, 0x20828dfb, 0x3ad7db53, 
    0x4d9f843b, 0xd1c1fb9f, 0x24fab215, 0xdaf1ba13, 
    0x999269a1, 0xf6e36b05, 0x3570b03c, 0x6fb98852, 
    0x7a7e0d35, 0x68dabe3b, 0x45e8c853, 0x90ba5e4f, 
    0x7888b1d1, 0xd0624813, 0x32e335c5, 0x30fb0102, 
    0x76286a0e, 0x430b920b, 0x20386733, 0x98c6c339, 
    0x445caa1f, 0x8f2658ef, 0x4e74829d, 0xbc783db0, 
    0x4ea609b2, 0xade42842, 0x35e2b2ef, 0x10e4080c, 
    0x8b667323, 0xe7da828b, 0x9f509fda, 0xa8e904da, 
    0x6ddbd31f, 0x6cf393e3, 0xcb5be69f, 0x9a729203, 
    0x0801a984, 0xa09b0cab, 0x1d6f0b45, 0x63b169eb, 
    0x5ce340fb, 0xc4ce898f, 0xb63c449c, 0x1829d69f, 
    0x8ff80d01, 0x70078ad0, 0xf4f15757, 0x2b659b3e, 
    0xef8991fb, 0x886c055c, 0xe025b833, 0x43421f34, 
    0x35ecb3c3, 0x0f2d2a12, 0xb82f78a8, 0x192706aa, 
    0xb2c6d9f3, 0xc2d8f8df, 0x76479b0f, 0x9f626c28, 
    0x9c7f6cb8, 0x427cdae8, 0x58e1eac2, 0x0c80f7f8, 
    0x243471b7, 0x619e4a4f, 0xdfa95d07, 0x7fcd6fdd, 
    0xc5914ddf, 0xe3517d0d, 0x51363d6a, 0x0464520d, 
    0xc5030747, 0x77597da5, 0xa4b33c73, 0x40399e63, 
    0x957159f4, 0x48af9162, 0x9bd544c8, 0xa5ede017, 
    0xcca2a5bf, 0x6c9dcb35, 0x88541535, 0x5456e0e8, 
    0xa3e7e574, 0x0ce71ace, 0x48ab2a26, 0xf801e568, 
    0x9c46ed43, 0x9a645ed7, 0xefaa1bef, 0xf2724625, 
    0x11e09a58, 0x38b2d6ff, 0xb3f20e82, 0x6c7f60e3, 
    0xa09465f0, 0xceac76e8, 0x7b5c5f6e, 0x64270c2c, 
    0x9c5132e5, 0x2b922941, 0xfe9841ca, 0x8bad491a, 
    0x1d32d8e1, 0xc6a98cd4, 0x1a5a452f, 0xf6efb7bb, 
};
