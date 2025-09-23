// binary generation

#include "iron/iron.h"
#include "xr.h"
#include "../mir/mir.h"

u8 get_register(FeFunc* f, FeInst* inst) {
    return f->vregs->at[inst->vr_def].real;
}

static inline XrMachInst imm_operate(u8 opcode, u8 ra, u8 rb, u16 imm) {
    return (XrMachInst){
        .opcode = opcode,
        .ra = ra,
        .rb = rb,
        .imm = imm,
    };
}

static XrMachInst arithi_to_mach(FeFunc* f, FeInst* inst) {
    u8 opcode;
    switch (inst->kind) {
    case XR_ADDI: opcode = 0x3C; break;
    case XR_SUBI: opcode = 0x34; break;
    case XR_SLTI: opcode = 0x2C; break;
    case XR_SLTI_S: opcode = 0x24; break;
    case XR_ANDI: opcode = 0x1C; break;
    case XR_XORI: opcode = 0x14; break;
    case XR_ORI: opcode = 0x0C; break;
    case XR_LUI: opcode = 0x04; break;
    default:
        FE_ASSERT(false);
    }
    
    return imm_operate(
        opcode,
        get_register(f, inst),
        get_register(f, inst->inputs[0]),
        fe_extra(inst, XrInstImm)->imm
    );
};

// if necessary, insert relocations
static void insert_relocations(FeMirAssemblerContext* ctx, FeInst* inst) {

}

static XrMachInst node_to_mach(FeFunc* f, FeInst* inst) {
    switch (inst->kind) {
    case FE__ROOT:
    case FE__MACH_RETURN:
    case FE__MACH_REG:
        return XR_MACH_INST_NULL;
    case XR_ADDI ... XR_LUI:
        return arithi_to_mach(f, inst);
    case XR_P_RET:
        return imm_operate(0x38, XR_GPR_ZERO, XR_GPR_LR, 0);
    default:
        FE_CRASH("unhandled translation");
    }
}
