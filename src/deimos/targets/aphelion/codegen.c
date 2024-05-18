#include "aphelion.h"
#include "target.h"
#include "ir.h"
#include "ptrmap.h"

PtrMap ir_to_vreg = {0};


AsmModule* aphelion_translate_module(IR_Module* irmod) {
    AsmModule* mod = asm_new_module(&aphelion_target_info);
}

AsmFunction* aphelion_translate_function(AsmModule* m, IR_Function* f) {

    if (ir_to_vreg.keys == NULL) {
        ptrmap_init(&ir_to_vreg, 100);
    }

    ptrmap_reset(&ir_to_vreg);
}

AsmBlock* aphelion_translate_block(AsmModule* m, AsmFunction* f, IR_Function* ir_f, IR_BasicBlock* ir_bb) {

    AsmBlock* b = asm_new_block(f, ir_bb->name);

    for_urange(ir_index, 0, ir_bb->len) {
        IR* raw_ir = ir_bb->at[ir_index];

        switch (raw_ir->tag) {
        
        case IR_PARAMVAL: {
            IR_ParamVal* ir = (IR_ParamVal*) raw_ir;

            // select callign convention register - TODO dont make this hardcoded
            VReg* src = asm_new_vreg(m, APHEL_REGCLASS_GPR);
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
            assert(ir->rhs->T->tag != TYPE_I64 && ir->lhs->T->tag != TYPE_I64);

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
        default:
            CRASH("unhandled ir type");
        }
    }


}