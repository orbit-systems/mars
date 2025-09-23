#include "iron/iron.h"
#include <stdio.h>
#include <string.h>

void fe_vrbuf_init(FeVRegBuffer* buf, usize cap) {
    if (cap < 2) cap = 2;
    buf->len = 0;
    buf->cap = cap;
    buf->at = fe_malloc(cap * sizeof(buf->at[0]));
}

void fe_vrbuf_clear(FeVRegBuffer* buf) {
    // memset(buf->at, 0, buf->len * sizeof(buf->at[0]));
    buf->len = 0;
}

void fe_vrbuf_destroy(FeVRegBuffer* buf) {
    fe_free(buf->at);
    *buf = (FeVRegBuffer){};
}

FeVReg fe_vreg_new(FeVRegBuffer* buf, FeInst* def, FeBlock* def_block, u8 class) {
    if (buf->len == buf->cap) {
        buf->cap += buf->cap >> 1;
        buf->at = fe_realloc(buf->at, buf->cap * sizeof(buf->at[0]));
    }
    FeVReg vr = buf->len++;
    buf->at[vr].class = class;
    buf->at[vr].def = def;
    buf->at[vr].def_block = def_block;
    buf->at[vr].real = FE_VREG_REAL_UNASSIGNED;
    buf->at[vr].hint = FE_VREG_NONE;
    buf->at[vr].is_phi_out = def->kind == FE_PHI;
    def->vr_def = vr;
    return vr;
}

FeVirtualReg* fe_vreg(FeVRegBuffer* buf, FeVReg vr) {
    if (vr == FE_VREG_NONE) return nullptr;
    return &buf->at[vr];
}

static void insert_upsilon(FeFunc* f) {
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->kind != FE_PHI) continue;

            FE_ASSERT(false);

            // usize len = fe_extra(inst, FeInstPhi)->len;
            // FeInst** srcs = fe_extra(inst, FeInstPhi)->vals;
            // FeBlock** blocks = fe_extra(inst, FeInstPhi)->blocks;

            // // add upsilon nodes
            // for_n(i, 0, len) {
            //     FeInst* upsilon = fe_inst_unop(f, inst->ty, FE__MACH_UPSILON, srcs[i]);
            //     fe_insert_before(blocks[i]->bookend->prev, upsilon);
            //     srcs[i] = upsilon;
            // }
        }
    }
}

void fe_codegen(FeFunc* f) {

    insert_upsilon(f);

    const FeTarget* target = f->mod->target;

    fe_stack_calculate_size(f);

    fe_opt_compact_ids(f);


    // create mapping from old values to new values
    // [old_inst->id] == new_inst
    struct {
        FeInst* from;
        FeInstChain selected;
    }* value_map = fe_malloc(sizeof(value_map[0]) * f->max_id);
    memset(value_map, 0, sizeof(value_map[0]) * f->max_id);
    usize value_map_len = f->max_id;

    // isel
    for_blocks(block, f) {
        for_inst(inst, block) {
            FeInstChain selected = target->isel(f, block, inst);
            value_map[inst->id].from = inst;
            value_map[inst->id].selected = selected;
        }
    }

    // replace
    for_n (i, 0, value_map_len) {
        FeInst* from = value_map[i].from;
        FeInstChain selected = value_map[i].selected;

        if (from == nullptr || selected.begin == nullptr) {
            continue;
        }
        // fe_chain_replace_pos(from, selected);
        fe_insert_chain_after(from, selected);
    }

    // fe_opt_compact_ids(f);

    // replace uses
    for_n (i, 0, value_map_len) {
        FeInst* from = value_map[i].from;
        FeInstChain selected = value_map[i].selected;

        if (from == nullptr || selected.begin == nullptr) {
            continue;
        }

        fe_replace_uses(f, from, selected.end);
    }

    // destroy old insts
    for_n (i, 0, value_map_len) {
        FeInst* from = value_map[i].from;
        if (from == nullptr || from->use_len != 0) {
            continue;
        }

        fe_inst_destroy(f, from);
    }

    fe_opt_tdce(f);

    fe_opt_compact_ids(f);

    fe_free(value_map);

    // create virtual registers for instructions that dont have them yet
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->kind == FE__MACH_UPSILON) continue;
            if ((inst->ty != FE_TY_VOID && inst->ty != FE_TY_TUPLE) && inst->vr_def == FE_VREG_NONE) {
                // chose regclass based on inst kind and type
                FeRegClass regclass = target->choose_regclass(inst->kind, inst->ty);
                inst->vr_def = fe_vreg_new(f->vregs, inst, block, regclass);
            }
        }
    }
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->kind != FE_PHI) continue;

            FE_ASSERT(false);

        }
    }

    fe_regalloc_basic(f);

    fe_opt_post_regalloc(f);
}
