#include "aphelion.h"
#include "target.h"
#include "ir.h"
#include "ptrmap.h"

PtrMap ir_to_vreg = {0};

AsmFunction* aphelion_translate_function(AsmModule* m, IR_Function* f);
AsmBlock* aphelion_translate_block(AsmModule* m, AsmFunction* f, IR_Function* ir_f, IR_BasicBlock* ir_bb);

AsmModule* aphelion_translate_module(IR_Module* irmod) {
    AsmModule* mod = asm_new_module(&aphelion_target_info);
    general_warning("FIXME: add globals support");
    for_urange(i, 0, irmod->functions_len) {
        aphelion_translate_function(mod, irmod->functions[i]);
    }

    asm_printer(mod);
}

AsmFunction* aphelion_translate_function(AsmModule* m, IR_Function* f) {
    if (ir_to_vreg.keys == NULL) {
        ptrmap_init(&ir_to_vreg, 100);
    }

    AsmFunction* new_func = asm_new_function(m, ir_sym_to_asm_sym(m, f->sym));
    new_func->blocks = malloc(sizeof(*new_func->blocks) * f->blocks.len);
    new_func->num_blocks = f->blocks.len;
    for_urange(i, 0, f->blocks.len) {
        AsmBlock* new_block = aphelion_translate_block(m, new_func, f, f->blocks.at[i]);
        new_func->blocks[i] = new_block;
    }

    ptrmap_reset(&ir_to_vreg);
    return new_func;
}

AsmBlock* aphelion_translate_block(AsmModule* m, AsmFunction* f, IR_Function* ir_f, IR_BasicBlock* ir_bb) {
    AsmBlock* b = asm_new_block(f, ir_bb->name);

    for_urange(ir_index, 0, ir_bb->len) {
        IR* raw_ir = ir_bb->at[ir_index];

        switch (raw_ir->tag) {
        
        case IR_PARAMVAL: {
            IR_ParamVal* ir = (IR_ParamVal*) raw_ir;

            assert(ir_f->params[ir->param_idx]->T->tag == TYPE_I64);

            // select calling convention register - TODO dont make this hardcoded
            VReg* src = asm_new_vreg(m, APHEL_REGCLASS_GPR);
            src->special = VREG_PARAMVAL;
            switch (ir->param_idx) {
            case 0: src->real = APHEL_GPR_RG; break;
            case 1: src->real = APHEL_GPR_RH; break;
            case 2: src->real = APHEL_GPR_RI; break;
            case 3: src->real = APHEL_GPR_RJ; break;
            case 4: src->real = APHEL_GPR_RK; break;
            default:
                CRASH("paramval index too big");
            }
            
            VReg* dest = asm_new_vreg(m, APHEL_REGCLASS_GPR);
            ptrmap_put(&ir_to_vreg, ir, dest);

            AsmInst* mov = asm_new_inst(m, APHEL_INST_MOV);
            mov->ins[0] = src;
            mov->outs[0] = dest;

            asm_add_inst(b, mov);
        } break;
        case IR_ADD: {
            IR_BinOp* ir = (IR_BinOp*) raw_ir;
            
            assert(ir->rhs->tag != IR_CONST && ir->lhs->tag != IR_CONST);
            assert(ir->rhs->T->tag == TYPE_I64 && ir->lhs->T->tag == TYPE_I64);


            VReg* lhs = ptrmap_get(&ir_to_vreg, ir->lhs);
            VReg* rhs = ptrmap_get(&ir_to_vreg, ir->rhs);
            assert(lhs && rhs);

            VReg* out = asm_new_vreg(m, APHEL_REGCLASS_GPR);
            ptrmap_put(&ir_to_vreg, ir, out);

            AsmInst* addr = asm_new_inst(m, APHEL_INST_ADDR);
            addr->ins[0] = lhs;
            addr->ins[1] = rhs;
            addr->outs[0] = out;

            asm_add_inst(b, addr);
        } break;
        case IR_RETURNVAL: {
            IR_ReturnVal* ir = (IR_ReturnVal*) raw_ir;

            VReg* out = asm_new_vreg(m, APHEL_REGCLASS_GPR);
            ptrmap_put(&ir_to_vreg, ir, out); // i dont think this actually needs to be put in

            VReg* in = ptrmap_get(&ir_to_vreg, ir->source);

            AsmInst* retval = asm_new_inst(m, APHEL_INST_MOV);
            retval->ins[0] = in;
            retval->outs[0] = out;

            asm_add_inst(b, retval);
        } break;
        case IR_RETURN: {
            AsmInst* ret = asm_new_inst(m, APHEL_INST_RET);
            asm_add_inst(b, ret);
        } break;
        default:
            CRASH("unhandled ir type");
        }
    }
    return b;
}