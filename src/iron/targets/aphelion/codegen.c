#include "iron.h"
#include "aphelion.h"
#include "ptrmap.h"
#include "regalloc.h"

PtrMap air_to_vreg = {0};

static AsmFunction* aphelion_translate_function(AsmModule* m, FeFunction* f);
static AsmBlock* aphelion_translate_block(AsmModule* m, AsmFunction* f, FeFunction* ir_f, FeBasicBlock* ir_bb);

static void aphelion_translate_module(FeModule* mod) {
    FeModule* ir_mod = mod->ir_module;
    AsmModule* asm_mod = mod->asm_module;
    general_warning("FIXME: add globals support");
    for_urange(i, 0, ir_mod->functions_len) {
        AsmFunction* f = aphelion_translate_function(asm_mod, ir_mod->functions[i]);
    }

    asm_printer(asm_mod, true);
    for_urange(i, 0, asm_mod->functions_len) {
        shit_regalloc(asm_mod, asm_mod->functions[i]);
    }
    asm_printer(asm_mod, true);

}

static AsmFunction* aphelion_translate_function(AsmModule* m, FeFunction* f) {
    if (air_to_vreg.keys == NULL) {
        ptrmap_init(&air_to_vreg, 100);
    }

    AsmFunction* new_func = asm_new_function(m, air_sym_to_asm_sym(m, f->sym));
    new_func->blocks = mars_alloc(sizeof(*new_func->blocks) * f->blocks.len);
    new_func->num_blocks = f->blocks.len;
    for_urange(i, 0, f->blocks.len) {
        AsmBlock* new_block = aphelion_translate_block(m, new_func, f, f->blocks.at[i]);
        new_func->blocks[i] = new_block;
    }

    ptrmap_reset(&air_to_vreg);
    return new_func;
}

static AsmBlock* aphelion_translate_block(AsmModule* m, AsmFunction* f, FeFunction* ir_f, FeBasicBlock* ir_bb) {
    AsmBlock* b = asm_new_block(f, ir_bb->name);

    for_urange(air_index, 0, ir_bb->len) {
        FeInst* raw_ir = ir_bb->at[air_index];

        switch (raw_ir->tag) {
        
        case FE_INST_PARAMVAL: {
            FeParamVal* ir = (FeParamVal*) raw_ir;

            assert(ir_f->params[ir->param_idx]->T->kind == FE_I64);

            // select calling convention register - TODO dont make this hardcoded
            FeVReg* src = asm_new_vreg(m, f, APHEL_REGCLASS_GPR);
            src->special = VSPEC_PARAMVAL;
            switch (ir->param_idx) {
            case 0: src->real = APHEL_GPR_RG; break;
            case 1: src->real = APHEL_GPR_RH; break;
            case 2: src->real = APHEL_GPR_RI; break;
            case 3: src->real = APHEL_GPR_RJ; break;
            case 4: src->real = APHEL_GPR_RK; break;
            default:
                CRASH("paramval index too big");
            }
            
            FeVReg* dest = asm_new_vreg(m, f, APHEL_REGCLASS_GPR);
            ptrmap_put(&air_to_vreg, ir, dest);
            dest->hint = src->real;

            FeAsmInst* mov = asm_new_inst(m, APHEL_INST_MOV);
            mov->ins[0] = src;
            mov->outs[0] = dest;

            asm_add_inst(b, mov);
        } break;
        case FE_INST_ADD: {
            FeBinop* ir = (FeBinop*) raw_ir;
            
            assert(ir->rhs->tag != FE_INST_CONST && ir->lhs->tag != FE_INST_CONST);
            assert(ir->rhs->T->kind == FE_I64 && ir->lhs->T->kind == FE_I64);


            FeVReg* lhs = ptrmap_get(&air_to_vreg, ir->lhs);
            FeVReg* rhs = ptrmap_get(&air_to_vreg, ir->rhs);
            assert(lhs && rhs);

            FeVReg* out = asm_new_vreg(m, f, APHEL_REGCLASS_GPR);
            ptrmap_put(&air_to_vreg, ir, out);

            FeAsmInst* addr = asm_new_inst(m, APHEL_INST_ADDR);
            addr->ins[0] = lhs;
            addr->ins[1] = rhs;
            addr->outs[0] = out;

            asm_add_inst(b, addr);
        } break;
        case FE_INST_MUL: {
            FeBinop* ir = (FeBinop*) raw_ir;
            
            assert(ir->rhs->tag != FE_INST_CONST && ir->lhs->tag != FE_INST_CONST);
            assert(ir->rhs->T->kind == FE_I64 && ir->lhs->T->kind == FE_I64);


            FeVReg* lhs = ptrmap_get(&air_to_vreg, ir->lhs);
            FeVReg* rhs = ptrmap_get(&air_to_vreg, ir->rhs);
            assert(lhs && rhs);

            FeVReg* out = asm_new_vreg(m, f, APHEL_REGCLASS_GPR);
            ptrmap_put(&air_to_vreg, ir, out);

            FeAsmInst* addr = asm_new_inst(m, APHEL_INST_IMULR);
            addr->ins[0] = lhs;
            addr->ins[1] = rhs;
            addr->outs[0] = out;

            asm_add_inst(b, addr);
        } break;
        case FE_INST_RETURNVAL: {
            FeReturnVal* ir = (FeReturnVal*) raw_ir;

            FeVReg* out = asm_new_vreg(m, f, APHEL_REGCLASS_GPR);
            ptrmap_put(&air_to_vreg, ir, out); // i dont think this actually needs to be put in
            
            out->special = VSPEC_RETURNVAL;
            switch (ir->return_idx) {
            case 0: out->real = APHEL_GPR_RG; break;
            case 1: out->real = APHEL_GPR_RH; break;
            case 2: out->real = APHEL_GPR_RI; break;
            case 3: out->real = APHEL_GPR_RJ; break;
            case 4: out->real = APHEL_GPR_RK; break;
            default:
                CRASH("returnval index too big");
            }

            FeVReg* in = ptrmap_get(&air_to_vreg, ir->source);
            in->hint = out->real;

            FeAsmInst* retval = asm_new_inst(m, APHEL_INST_MOV);
            retval->ins[0] = in;
            retval->outs[0] = out;

            asm_add_inst(b, retval);
        } break;
        case FE_INST_RETURN: {
            FeAsmInst* ret = asm_new_inst(m, APHEL_INST_RET);
            asm_add_inst(b, ret);
        } break;
        default:
            CRASH("unhandled ir type");
        }
    }
    return b;
}

FePass pass_aphelion_codegen = {
    .name = "aphelion::codegen",
    .callback = aphelion_translate_module,
};