#include "iron/iron.h"
#include <stdio.h>

bool is_canon_def(FeVirtualReg* inst_out, FeInst* inst) {
    return (inst->kind == FE__MACH_UPSILON)
        || inst_out->def == inst;
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
                    if (!succ_live_in_vr->is_phi_input && succ_live_in_vr->def_block != block) {
                        changed |= add_live_in(block->live, succ_live_in);
                    }
                }
            }
        }
    }
}

typedef struct ColorNode {
    FeVirtualReg* this;
    FeVReg* interferes;
    FeVReg* hints;
    u16 inter_len;
    u16 hint_len;
    FeVReg vr;
} ColorNode;

static void add_interference(ColorNode* cnode, FeVReg interferes) {
    if (cnode->vr == interferes) {
        return;
    }
    
    for_n(i, 0, cnode->inter_len) {
        if (cnode->interferes[i] == interferes) {
            return;
        }
    }

    cnode->interferes[cnode->inter_len] = interferes;
    cnode->inter_len += 1;
}

static void remove_interference(ColorNode* cnode, FeVReg interferes) {
    for_n(i, 0, cnode->inter_len) {
        if (cnode->interferes[i] == interferes) {
            cnode->interferes[i] = cnode->interferes[--cnode->inter_len];
            return;
        }
    }
}

static void add_hint(ColorNode* cnodes, ColorNode* cnode, FeVReg hint) {
    // if (cnode->vr == hint) {
    //     return;
    // }

    for_n(i, 0, cnode->hint_len) {
        if (cnode->hints[i] == hint) {
            return;
        }
    }

    cnode->hints[cnode->hint_len] = hint;
    cnode->hint_len += 1;

    for_n(i, 0, cnode->hint_len - 1) {
        add_hint(cnodes, &cnodes[cnode->hints[i]], hint);
    }
}

// try to take a hint, return true if taken, false if not taken
// static bool try_vr_hint(FeVRegBuffer* vbuf, ColorNode* cnode) {
//     if (vbuf->at[cnode->this->hint].real == FE_VREG_REAL_UNASSIGNED) {
//         return false;
//     }

//     for_n(i, 0, cnode->inter_len) {
//         if (vbuf->at[cnode->interferes[i]].real == vbuf->at[cnode->this->hint].real) {
//             return false;
//         }
//     }

//     cnode->this->real = vbuf->at[cnode->this->hint].real;
//     return true;
// }

void fe_regalloc_basic(FeFunc* f) {

    FeVRegBuffer* vbuf = f->vregs;
    const FeTarget* target = f->mod->target;
    FE_ASSERT(target->num_regclasses == 1);

    calculate_liveness(f);

    // all of this is horribly inefficient
    // i just want it to work right now
    ColorNode* color_nodes = fe_malloc(vbuf->len * sizeof(ColorNode));

    for_n(i, 0, vbuf->len) {
        color_nodes[i].this = &vbuf->at[i];
        color_nodes[i].hints = fe_malloc(vbuf->len * sizeof(FeVReg));
        color_nodes[i].interferes = fe_malloc(vbuf->len * sizeof(FeVReg));
        color_nodes[i].inter_len = 0;
        color_nodes[i].hint_len = 0;
    }

    // hints!
    for_blocks(block, f) {
        for_inst(inst, block) {
            if (!fe_inst_has_trait(inst->kind, FE_TRAIT_REG_MOV_HINT)) {
                continue;
            }

            // FeVirtualReg* inst_vr = fe_vreg(f->vregs, inst->vr_def);

            // hint input and output to each other
            FeInst* input = inst->inputs[0];
            // FeVirtualReg* input_vr = fe_vreg(f->vregs, input->vr_def);

            FeVReg src = input->vr_def;
            FeVReg dst = inst->vr_def;
            ColorNode* src_n = &color_nodes[src];
            ColorNode* dst_n = &color_nodes[dst];

            add_hint(color_nodes, dst_n, src);
            add_hint(color_nodes, src_n, dst);
        }
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
            // if we come across a canonical definition of a vreg, end its life
            FeVReg vr = inst->vr_def;
            if (vr != FE_VREG_NONE && is_canon_def(&vbuf->at[vr], inst)) {
                // printf("end %d at inst %s\n", vr, fe_inst_name(target, inst->kind));
                live_now[vr] = false;
            }
            
            // start life of inputs
            for_n(i, 0, inst->in_len) {
                FeInst* inst_input = inst->inputs[i];
                if (inst_input->vr_def != FE_VREG_NONE) {
                    // printf("begin %d at inst %s\n", inst_input->vr_def, fe_inst_name(target, inst->kind));
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

    // remove interferences that are also hints
    for_n(vr, 0, vbuf->len) {
        ColorNode* cnode = &color_nodes[vr];
        for_n(h, 0, cnode->hint_len) {
            FeVReg hint = cnode->hints[h];
            remove_interference(cnode, hint);
        }
    }

    // debug print interference graph
    // for_n(vr, 0, vbuf->len) {
    //     ColorNode* cnode = &color_nodes[vr];
    //     for_n(i, 0, cnode->inter_len) {
    //         printf("interference %zu ~> %d\n", vr, cnode->interferes[i]);
    //     }
    // }
    
    // color!
    

    FeCompactMap cnodes;
    fe_cmap_init(&cnodes);

    FeCompactMap high_priority_cnodes;
    fe_cmap_init(&high_priority_cnodes);
    
    for_n(vr, 0, vbuf->len) {
        ColorNode* cnode = &color_nodes[vr];
        if (cnode->hint_len) {
            fe_cmap_put(&high_priority_cnodes, vr, (uintptr_t)cnode);
        }
        fe_cmap_put(&cnodes, vr, (uintptr_t)cnode);
    }
    
    while (true) {
        ColorNode* cnode;
        cnode = (ColorNode*)fe_cmap_pop_next(&high_priority_cnodes);
        if (cnode == nullptr) {
            cnode = (ColorNode*)fe_cmap_pop_next(&cnodes);
            if (cnode == nullptr) {
                break;
            }
        }

        // skip if pre-colored
        if (cnode->this->real != FE_VREG_REAL_UNASSIGNED) {
            continue;
        }

        bool exit_early = false;
        // attempt to take hint if exists
        for_n (hi, 0, cnode->hint_len) {
            FeVReg hint = cnode->hints[hi];

            if (vbuf->at[hint].real != FE_VREG_REAL_UNASSIGNED) {
                // take the hint.

                // to take the hint, we have to see if it interferes with anything.
                bool can_take_hint = true;
                for_n(i, 0, cnode->inter_len) {
                    if (vbuf->at[cnode->interferes[i]].real == vbuf->at[hint].real) {
                        // hint cannot be taken because it interferes with already colored things
                        can_take_hint = false;
                        break;
                    }
                }

                if (!can_take_hint) {
                    continue;
                }

                cnode->this->real = vbuf->at[hint].real;
                for_n(i, 0, cnode->hint_len) {
                    fe_cmap_put(&high_priority_cnodes, 
                        cnode->hints[i],
                        (uintptr_t) &color_nodes[cnode->hints[i]]
                    );
                }
                exit_early = true;
                break;
            } else {
                // hinted vreg hasnt been colored yet.
                // wait for it to be colored ONLY if it will be colored later.
                if (fe_cmap_contains_key(&high_priority_cnodes, hint) || 
                    fe_cmap_contains_key(&cnodes, hint)) 
                {
                    exit_early = true;
                    break;
                }
            }
        }
        if (exit_early) {
            continue;
        }

        // we gotta chose a register that doesnt interfere with anything already allocated.
        for_n(real_candidate, 0, target->regclass_lens[cnode->this->class]) {
            if (target->reg_status(f->sig->cconv, cnode->this->class, real_candidate) == FE_REG_UNUSABLE) {
                // cant touch this, we gotta continue
                continue;
            }
            // printf("try alloc %zu\n", real_candidate);

            for_n(i, 0, cnode->inter_len) {

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
        for_n(i, 0, cnode->hint_len) {
            fe_cmap_put(&high_priority_cnodes, 
                cnode->hints[i],
                (uintptr_t) &color_nodes[cnode->hints[i]]
            );
        }
    }

    fe_free(color_nodes);
}
