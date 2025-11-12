#include "iron/iron.h"
#include "xr.h"

static FeInst* xr_inst(FeFunc* f, XrInstKind kind, usize input_len, usize extra_size) {
    FeInst* inst = fe_inst_new(f, input_len, extra_size);
    inst->kind = kind;
    inst->vr_def = FE_VREG_NONE;
    return inst;
}

static FeInst* mach_reg(FeFunc* f, FeBlock* b, XrGpr real) {
    FeInst* src_reg = fe_inst_new(f, 0, 0);
    src_reg->kind = FE__MACH_REG;
    src_reg->ty = FE_TY_I32;
    src_reg->vr_def = fe_vreg_new(f->vregs, src_reg, b, XR_REGCLASS_GPR);
    fe_vreg(f->vregs, src_reg->vr_def)->real = real;
    return src_reg;
}

static void assign_real_reg(FeFunc* f, FeBlock* b, FeInst* inst, XrGpr real) {
    inst->vr_def = fe_vreg_new(f->vregs, inst, b, XR_REGCLASS_GPR);
    fe_vreg(f->vregs, inst->vr_def)->real = real;
}

static bool is_const_u5(FeInst* inst) {
    if (inst->kind != FE_CONST) {
        return false;
    }
    u64 val = fe_extra(inst, FeInstConst)->val;
    return (val & 0xFFFF) == val;
}

static bool is_const_zero(FeInst* inst) {
    if (inst->kind != FE_CONST) {
        return false;
    }
    u64 val = fe_extra(inst, FeInstConst)->val;
    return val == 0;
}

static bool is_const_u16(FeInst* inst) {
    if (inst->kind != FE_CONST) {
        return false;
    }
    u64 val = fe_extra(inst, FeInstConst)->val;
    return (val & 0xFFFF) == val;
}

static bool is_neg_const_u16(FeInst* inst) {
    if (inst->kind != FE_CONST) {
        return false;
    }
    u64 val = -fe_extra(inst, FeInstConst)->val;
    return (val & 0xFFFF) == val;
}

static bool is_const_u16_offset(FeInst* inst) {
    if (inst->kind != FE_IADD) {
        return false;
    }
    return is_const_u16(inst->inputs[1]);
}

static u64 const_u16_offset(FeInst* inst) {
    return fe_extra(inst->inputs[1], FeInstConst)->val;
}

static u64 const_val(FeInst* inst) {
    return fe_extra(inst, FeInstConst)->val;
}

static FeInstChain get_parameter(FeFunc* f, FeBlock* entry, usize index) {
    switch (f->sig->cconv) {
    case FE_CCONV_ANY:
    case FE_CCONV_JACKAL:
        break;
    default:
        FE_CRASH("unsupported cconv");
    }

    FeTy param_ty = f->sig->params[index].ty;

    if (index < 4) {
        // integer argument through registers.
        static u16 arg_regs[4] = {XR_GPR_A0, XR_GPR_A1, XR_GPR_A2, XR_GPR_A3};
        
        // create move from register
        FeInst* reg = mach_reg(f, entry, arg_regs[index]);
        reg->ty = param_ty;
        FeInst* mov = fe_inst_unop(f, param_ty, FE_MOV, reg);

        FeInstChain chain = fe_chain_new(reg);
        chain = fe_chain_append_end(chain, mov);
        return chain;
    } else {
        FE_CRASH("stack args so far unsupported");
    }
}

static FeInstChain store_returnval(FeFunc* f, FeBlock* exit, usize index, FeInst* value) {
    switch (f->sig->cconv) {
    case FE_CCONV_ANY:
    case FE_CCONV_JACKAL:
        break;
    default:
        FE_CRASH("unsupported cconv");
    }

    FeTy param_ty = f->sig->params[index].ty;

    if (index < 4) {
        // integer argument through registers.
        static u16 ret_regs[4] = {XR_GPR_A3, XR_GPR_A2, XR_GPR_A1, XR_GPR_A0};
        
        // create move to register
        FeInst* mov = fe_inst_unop(f, param_ty, FE__MACH_MOV, value);
        mov->vr_def = fe_vreg_new(f->vregs, mov, exit, XR_REGCLASS_GPR);
        fe_vreg(f->vregs, mov->vr_def)->real = ret_regs[index];

        FeInstChain chain = fe_chain_new(mov);
        return chain;
    } else {
        FE_CRASH("stack returns so far unsupported");
    }
}

FeInstChain fe_xr_isel(FeFunc* f, FeBlock* block, FeInst* inst) {
    switch (inst->kind) {
        case FE__ROOT:
        case FE__MACH_UPSILON:
        case FE_MEM_PHI:
        case FE_MEM_MERGE:
        case FE_MEM_BARRIER:
            return FE_EMPTY_CHAIN;
        case FE_PROJ: {
            if (inst->inputs[0]->kind == FE__ROOT) {
                // figure out what parameter we're looking at.
                usize index = fe_extra(inst, FeInstProj)->index;
                return get_parameter(f, block, index);
            }
            FE_CRASH("unknown proj selection");
        }
        case FE_STACK_ADDR: {
            FeStackItem* item = fe_extra(inst, FeInstStack)->item;
            
            FeInst* sp = mach_reg(f, block, XR_GPR_SP);
            FeInst* add = xr_inst(f, XR_ADDI, 1, sizeof(XrInstImm));
            add->ty = FE_TY_I32;
            fe_set_input(f, add, 0, sp);
            fe_extra(add, XrInstImm)->imm = item->_offset;

            FeInstChain chain = fe_chain_new(sp);
            chain = fe_chain_append_end(chain, add);
            return chain;
        }
        case FE_CONST: {
            if (is_const_zero(inst)) {
                // just use the zero register lmao
                FeInst* zero = mach_reg(f, block, XR_GPR_ZERO);
                FeInstChain chain = fe_chain_new(zero);
                return chain;
            }

            if (is_const_u16(inst)) {
                // create addi with zero
                FeInst* zero = mach_reg(f, block, XR_GPR_ZERO);
                FeInst* addi = xr_inst(f, XR_ADDI, 1, sizeof(XrInstImm));
                addi->ty = FE_TY_I32;
                fe_extra(addi, XrInstImm)->imm = const_val(inst);
                fe_set_input(f, addi, 0, zero);

                FeInstChain chain = fe_chain_new(zero);
                chain = fe_chain_append_end(chain, addi);
                return chain;
            }
            if (is_neg_const_u16(inst)) {
                FeInst* zero = mach_reg(f, block, XR_GPR_ZERO);
                FeInst* subi = xr_inst(f, XR_SUBI, 1, sizeof(XrInstImm));
                subi->ty = FE_TY_I32;
                fe_extra(subi, XrInstImm)->imm = -const_val(inst);
                fe_set_input(f, subi, 0, zero);
                
                FeInstChain chain = fe_chain_new(zero);
                chain = fe_chain_append_end(chain, subi);
                return chain;
            }

            FE_CRASH("constant too big! for now....");
        }

        // ARITHMETIC

        case FE_IADD: {
            if (is_const_u16(inst->inputs[1])) {
                FeInst* sel = xr_inst(f, XR_ADDI, 1, sizeof(XrInstImm));
                sel->ty = FE_TY_I32;
                fe_set_input(f, sel, 0, inst->inputs[0]);
                fe_extra(sel, XrInstImm)->imm = const_val(inst->inputs[1]);
                return fe_chain_new(sel);
            }
            if (is_neg_const_u16(inst->inputs[1])) {
                FeInst* sel = xr_inst(f, XR_SUBI, 1, sizeof(XrInstImm));
                sel->ty = FE_TY_I32;
                fe_set_input(f, sel, 0, inst->inputs[0]);
                fe_extra(sel, XrInstImm)->imm = -const_val(inst->inputs[1]);
                return fe_chain_new(sel);
            }
            FeInst* sel = xr_inst(f, XR_ADD, 2, sizeof(XrInstImm));
            sel->ty = FE_TY_I32;
            fe_set_input(f, sel, 0, inst->inputs[0]);
            fe_set_input(f, sel, 1, inst->inputs[1]);
            return fe_chain_new(sel);
        }
        case FE_ISUB: {
            if (is_const_u16(inst->inputs[1])) {
                FeInst* sel = xr_inst(f, XR_SUBI, 1, sizeof(XrInstImm));
                sel->ty = FE_TY_I32;
                fe_set_input(f, sel, 0, inst->inputs[0]);
                fe_extra(sel, XrInstImm)->imm = const_val(inst->inputs[1]);
                return fe_chain_new(sel);
            }
            FeInst* sel = xr_inst(f, XR_SUB, 2, sizeof(XrInstImm));
            sel->ty = FE_TY_I32;
            fe_set_input(f, sel, 0, inst->inputs[0]);
            fe_set_input(f, sel, 1, inst->inputs[1]);
            return fe_chain_new(sel);
        }
        case FE_IEQ: {
            // %1 = ieq %2, %3 

            // %4 = sub %2, %3
            // %1 = slti %4, 1 

            FeInst* sub = xr_inst(f, XR_SUB, 2, sizeof(XrInstImm));
            sub->ty = FE_TY_I32;
            fe_set_input(f, sub, 0, inst->inputs[0]);
            fe_set_input(f, sub, 1, inst->inputs[1]);

            FeInst* slti = xr_inst(f, XR_SLTI, 1, sizeof(XrInstImm));
            slti->ty = FE_TY_I32;
            fe_set_input(f, slti, 0, sub);
            fe_extra(slti, XrInstImm)->imm = 1;

            FeInstChain chain = fe_chain_new(sub);
            chain = fe_chain_append_end(chain, slti);
            return chain;
        }

        // MEMORY OPERATIONS

        case FE_STORE: {
            FeInst* ptr = inst->inputs[1];
            FeInst* val = inst->inputs[2];

            FE_ASSERT(val->ty == FE_TY_I32);

            // can put in small-immediate?
            if (is_const_u5(val) && const_val(val) != 0) {
                FeInst* store = xr_inst(f, XR_STORE32_SI, 1, sizeof(XrInstImm));
                store->ty = FE_TY_VOID;
                fe_extra(store, XrInstImm)->small = const_val(val);
                
                // is pointer a 16-bit offset?
                if (is_const_u16_offset(ptr)) {
                    fe_set_input(f, store, 0, ptr->inputs[0]);
                    fe_extra(store, XrInstImm)->imm = const_u16_offset(ptr);
                } else {
                    fe_set_input(f, store, 0, ptr);
                    fe_extra(store, XrInstImm)->imm = 0;
                }

                FeInstChain chain = fe_chain_new(store);
                return chain;
                // FE_CRASH("small immediate!");
            } else {
                FeInst* store = xr_inst(f, XR_STORE32_IO, 2, sizeof(XrInstImm));
                store->ty = FE_TY_VOID;
                fe_set_input(f, store, 1, val);
                
                // is pointer a 16-bit offset?
                if (is_const_u16_offset(ptr)) {
                    fe_set_input(f, store, 0, ptr->inputs[0]);
                    fe_extra(store, XrInstImm)->imm = const_u16_offset(ptr);
                } else {
                    fe_set_input(f, store, 0, ptr);
                    fe_extra(store, XrInstImm)->imm = 0;
                }
                
                FeInstChain chain = fe_chain_new(store);
                return chain;
            }
        }
        case FE_LOAD: {
            FeInst* ptr = inst->inputs[1];

            FeInst* load = xr_inst(f, XR_LOAD32_IO, 1, sizeof(XrInstImm));
            load->ty = inst->ty;
            fe_extra(load, XrInstImm)->imm = 0;
            
            fe_set_input(f, load, 0, ptr);
            
            FeInstChain chain = fe_chain_new(load);
            return chain;
        }

        // CONTROL FLOW
        case FE_PHI: {
            return FE_EMPTY_CHAIN;
        }
        case FE_BRANCH: {
            // very dumb pattern for now
            
            // branch %x, 1:, 2:
            // ->
            // beq %x, 1: (else 2:)

            FeInst* cond = inst->inputs[0];

            if (cond->kind == FE_IEQ && is_const_zero(cond->inputs[1])) {
                cond = cond->inputs[0];
            }
            FeInst* beq = xr_inst(f, XR_BEQ, 1, sizeof(XrInstBranch));
            fe_set_input(f, beq, 0, cond);
            fe_extra(beq, XrInstBranch)->if_true = fe_extra(inst, FeInstBranch)->if_true;
            fe_extra(beq, XrInstBranch)->fake_if_false = fe_extra(inst, FeInstBranch)->if_false;
            
            FeInstChain chain = fe_chain_new(beq);
            return chain;
        }
        case FE_JUMP: {
            FeInst* b = xr_inst(f, XR_P_B, 0, sizeof(XrInstJump));
            fe_extra(b, XrInstJump)->target = fe_extra(inst, FeInstJump)->to;
            
            FeInstChain chain = fe_chain_new(b);
            return chain;
        }
        case FE_RETURN: {
            FeInstChain chain = FE_EMPTY_CHAIN;
            
            // store the return values
            for_n(i, 1, inst->in_len) {
                FeInstChain retval = store_returnval(f, block, i - 1, inst->inputs[i]);
                chain = fe_chain_concat(chain, retval);
            }
            
            // actual return instruction lmao
            FeInst* ret = xr_inst(f, XR_P_RET, 0, 0);

            chain = fe_chain_append_end(chain, ret);
            return chain;
        }
        default:
            FE_CRASH("cannot select from inst %s (%d)", fe_inst_name(f->mod->target, inst->kind), inst->kind);
    }
}

void fe_xr_post_regalloc_reduce(FeFunc* f) {
    for_blocks(block, f) {
        for_inst(inst, block) {
            switch (inst->kind) {
            case FE__MACH_UPSILON:
            case FE__MACH_MOV: {
                // replace with single mov
                FeInst* mov = xr_inst(f, XR_P_MOV, 1, 0);
                mov->ty = inst->ty;
                mov->vr_def = inst->vr_def;
                fe_set_input(f, mov, 0, inst->inputs[0]);
                fe_insert_after(inst, mov);
                fe_replace_uses(f, inst, mov);
                fe_inst_destroy(f, inst);
                break;
            }
            default:
                break;
            }
        }
    }

    // fe_opt_tdce(f);
}
