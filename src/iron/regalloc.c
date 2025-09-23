#include "iron/iron.h"
#include <stdio.h>

bool is_canon_def(FeVirtualReg* inst_out, FeInst* inst) {
    // upsilons are a kind of fake move/definition that get created during codegen as inputs for phi nodes.
    return (inst->kind == FE__MACH_UPSILON || inst_out->def == inst);
}

static bool add_live_in(FeBlockLiveness* lv, FeVReg vr) {
    // check to see if its already in the live-in_set.
    for_n(i, 0, lv->in_len) {
        if (lv->in[i] == vr) {
            return false;
        }
    }
    // vr is not in live-in. add it.
    if (lv->in_len == lv->in_cap) {
        lv->in_cap += lv->in_cap >> 1;
        lv->in = fe_realloc(lv->in, sizeof(lv->in[0]) * lv->in_cap);
    }
    lv->in[lv->in_len++] = vr;
    return true;
}

static bool add_live_out(FeBlockLiveness* lv, FeVReg vr) {
    // check to see if its already in the live-out set.
    for_n(i, 0, lv->out_len) {
        if (lv->out[i] == vr) {
            return false;
        }
    }
    // vr is not in live-out. add it.
    if (lv->out_len == lv->out_cap) {
        lv->out_cap += lv->out_cap >> 1;
        lv->out = fe_realloc(lv->out, sizeof(lv->out[0]) * lv->out_cap);
    }
    lv->out[lv->out_len++] = vr;
    return true;
}

static void calculate_liveness(FeFunc* f) {
    const FeTarget* t = f->mod->target;

    // make sure cfg is updated.

    // give every basic block a liveness chunk.
    for_blocks(block, f) {
        FeBlockLiveness* lv = fe_zalloc(sizeof(FeBlockLiveness));
        lv->block = block;
        block->live = lv;

        // initialize live-in/live-out vectors
        lv->in_cap = 16;
        lv->out_cap = 16;
        lv->in = fe_malloc(sizeof(lv->in[0]) * lv->in_cap);
        lv->out = fe_malloc(sizeof(lv->out[0]) * lv->out_cap);
    }
    
    // initialize simple live-ins
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (inst->vr_def == FE_VREG_NONE) continue;
            
            FeInst** inputs = inst->inputs;
            for_n(i, 0, inst->in_len) {
                FeInst* input = inputs[i];
                FeVirtualReg* vr = fe_vreg(f->vregs, input->vr_def);
                if (input->kind == FE__MACH_UPSILON || vr->def_block != block) {
                    add_live_in(block->live, input->vr_def);
                }
            }
        }
    }

    // iterate over the blocks, refining liveness until everything is settled
    bool changed = true;
    while (changed) {
        changed = false;
        // block.live_out = block.live_out U successor0.live_in U successor1.live_in ...
        for_blocks(block, f) {
            for_n(succ_i, 0, block->succ_len) {
                FeBlock* succ = block->succ[succ_i];
                for_n(i, 0, succ->live->in_len) {
                    FeVReg succ_live_in = succ->live->in[i];
                    // add it to block.out
                    changed |= add_live_out(block->live, succ_live_in);
                    // if not defined in this block, add it block.in
                    FeVirtualReg* succ_live_in_vr = fe_vreg(f->vregs, succ_live_in);
                    if (!succ_live_in_vr->is_phi_out && succ_live_in_vr->def_block != block) {
                        changed |= add_live_in(block->live, succ_live_in);
                    }
                }
            }
        }
    }
}

typedef struct {
    FeVirtualReg* this;
    
    FeVReg* interferes;
    u32 len;
} ColorNode;


static void add_interference(ColorNode* cnode, FeVReg interferes) {
    for_n(i, 0, cnode->len) {
        if (cnode->interferes[i] == interferes) {
            return;
        }
    }

    cnode->interferes[cnode->len] = interferes;
    cnode->len += 1;
}

// try to take a hint, return true if taken, false if not taken
static bool try_vr_hint(FeVRegBuffer* vbuf, ColorNode* cnode) {
    // return false;
    for_n(i, 0, cnode->len) {
        if (vbuf->at[cnode->interferes[i]].real == vbuf->at[cnode->this->hint].real) {
            return false;
        }
    }

    cnode->this->real = vbuf->at[cnode->this->hint].real;
    return true;
}

void fe_regalloc_basic(FeFunc* f) {

    FeVRegBuffer* vbuf = f->vregs;
    const FeTarget* target = f->mod->target;
    FE_ASSERT(target->num_regclasses == 2); // including the NONE regclass

    calculate_liveness(f);

    // hints!
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (!fe_inst_has_trait(inst->kind, FE_TRAIT_REG_MOV_HINT)) {
                continue;
            }

            FeVirtualReg* inst_vr = fe_vreg(f->vregs, inst->vr_def);

            // hint input and output to each other
            FeInst* input = inst->inputs[0];
            FeVirtualReg* input_vr = fe_vreg(f->vregs, input->vr_def);

            inst_vr->hint = input->vr_def;
            input_vr->hint = inst->vr_def;
        }
    }

    ColorNode* color_nodes = fe_malloc(vbuf->len * sizeof(ColorNode));
    // memset(color_nodes, 0, vbuf->len * sizeof(ColorNode));

    // all of this is horribly inefficient
    // i just want it to work right now

    for_n(i, 0, vbuf->len) {
        color_nodes[i].this = &vbuf->at[i];
        color_nodes[i].interferes = fe_malloc(vbuf->len * sizeof(FeVReg));
        color_nodes[i].len = 0;
    }

    bool* live_now = fe_malloc(vbuf->len * sizeof(bool));
    memset(live_now, 0, vbuf->len * sizeof(bool));

    for_blocks(block, f) {
        // set initial live-outs
        memset(live_now, 0, vbuf->len * sizeof(bool));
        for_n(i, 0, block->live->out_len) {
            FeVReg live_out = block->live->out[i];
            live_now[live_out] = true;
            
        }

        // all of the initial live_outs interfere with each other
        for_n(i, 0, block->live->out_len) {
            FeVReg live_out_1 = block->live->out[i];
            for_n(j, 0, block->live->out_len) {
                FeVReg live_out_2 = block->live->out[j];
                if (live_out_1 == live_out_2) {
                    continue;
                }

                add_interference(&color_nodes[live_out_1], live_out_2);
            }
        }

        for_inst_reverse(inst, block) {
            // if we come across the 'canon' definition of a vreg, end its life
            FeVReg vr = inst->vr_def;
            if (vr != FE_VREG_NONE && is_canon_def(&vbuf->at[vr], inst)) {
                live_now[vr] = false;
            }
            
            // start life of inputs
            for_n(i, 0, inst->in_len) {
                FeInst* inst_input = inst->inputs[i];
                if (inst_input->vr_def != FE_VREG_NONE) {
                    live_now[inst_input->vr_def] = true;
                }
            }
            
            for_n(i, 0, vbuf->len) {
                if (!live_now[i]) {
                    continue;
                }
                for_n(j, 0, vbuf->len) {
                    if (!live_now[j] || i == j) {
                        continue;
                    }
                    add_interference(&color_nodes[i], j);
                }
            }
        }
    }

    // debug print interference graph
    for_n(vr, 0, vbuf->len) {
        ColorNode* cnode = &color_nodes[vr];
        for_n(i, 0, cnode->len) {
            printf("interference %zu ~> %d\n", vr, cnode->interferes[i]);
        }
    }

    // color!
    // if (false)
    for_n(vr, 0, vbuf->len) {
        ColorNode* cnode = &color_nodes[vr];

        // printf("vr %zu hint %d\n", vr, cnode->this->hint);

        // skip if pre-colored
        if (cnode->this->real != FE_VREG_REAL_UNASSIGNED) {
            continue;
        }

        // attempt to take hint if exists
        if (cnode->this->hint != FE_VREG_NONE) {
            // AND the hint isnt already assigned to something
            if (try_vr_hint(vbuf, cnode)) {
                // chosen!
                continue;
            }
        }

        // we gotta chose a register that doesnt interfere with anything already allocated.
        for_n(real_candidate, 0, target->regclass_lens[1]) {
            if (target->reg_status(f->sig->cconv, 1, real_candidate) == FE_REG_UNUSABLE) {
                // cant touch this, we gotta continue
                continue;
            }
            // printf("try alloc %zu\n", real_candidate);

            for_n(i, 0, cnode->len) {

                ColorNode* interferes = &color_nodes[cnode->interferes[i]];
                if (interferes->this->real == FE_VREG_REAL_UNASSIGNED) {
                    continue;
                }

                // has this real register already been chosen?
                if (interferes->this->real == real_candidate) {
                    // try the next one
                    goto next_candidate;
                }
            }

            // chosen!
            cnode->this->real = real_candidate;
            break;

            next_candidate:
        }
        if (cnode->this->real == FE_VREG_REAL_UNASSIGNED) {
            FE_CRASH("failed to allocate register");
        }
    }

    fe_free(color_nodes);
}
