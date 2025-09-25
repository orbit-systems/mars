#include "iron/iron.h"
#include "xr.h"

FeRegClass fe_xr_choose_regclass(FeInstKind kind, FeTy ty) {
    switch (ty) {
    case FE_TY_I8:
    case FE_TY_I16:
    case FE_TY_I32:
    case FE_TY_I64:
    case FE_TY_BOOL:
        return XR_REGCLASS_GPR;
    default:
        FE_CRASH("cant select regclass for type %s", fe_ty_name(ty));
    }
}

void fe_xr_print_inst(FeDataBuffer* db, FeFunc* f, FeInst* inst) {
    const char* name = nullptr;
    switch (inst->kind) {
    case XR_J:   name = "xr.j"; break;
    case XR_JAL: name = "xr.jal"; break;

    case XR_BEQ: name = "xr.beq"; break;
    case XR_BNE: name = "xr.bne"; break;
    case XR_BLT: name = "xr.blt"; break;
    case XR_BLE: name = "xr.ble"; break;
    case XR_BGT: name = "xr.bgt"; break;
    case XR_BGE: name = "xr.bge"; break;
    case XR_BPE: name = "xr.bpe"; break;
    case XR_BPO: name = "xr.bpo"; break;

    case XR_ADDI:   name = "xr.addi"; break;
    case XR_SUBI:   name = "xr.subi"; break;
    case XR_SLTI:   name = "xr.slti"; break;
    case XR_SLTI_S: name = "xr.slti-s"; break;
    case XR_ANDI:   name = "xr.andi"; break;
    case XR_XORI:   name = "xr.xori"; break;
    case XR_ORI:    name = "xr.ori"; break;
    case XR_LUI:    name = "xr.lui"; break;

    case XR_LOAD8_IO:   name = "xr.load8-io"; break;
    case XR_LOAD16_IO:  name = "xr.load16-io"; break;
    case XR_LOAD32_IO:  name = "xr.load32-io"; break;
    case XR_STORE8_IO:  name = "xr.store8-io"; break;
    case XR_STORE16_IO: name = "xr.store16-io"; break;
    case XR_STORE32_IO: name = "xr.store32-io"; break;

    case XR_LOAD8_RO:   name = "xr.load8-ro"; break;
    case XR_LOAD16_RO:  name = "xr.load16-ro"; break;
    case XR_LOAD32_RO:  name = "xr.load32-ro"; break;
    case XR_STORE8_RO:  name = "xr.store8-ro"; break;
    case XR_STORE16_RO: name = "xr.store16-ro"; break;
    case XR_STORE32_RO: name = "xr.store32-ro"; break;
    
    case XR_STORE8_SI:  name = "xr.store8-si"; break;
    case XR_STORE16_SI: name = "xr.store16-si"; break;
    case XR_STORE32_SI: name = "xr.store32-si"; break;
    case XR_JALR:       name = "xr.jalr"; break;
    case XR_ADR:        name = "xr.adr"; break;

    case XR_SHIFT: name = "xr.shift"; break;
    case XR_ADD:   name = "xr.add"; break;
    case XR_SUB:   name = "xr.sub"; break;
    case XR_SLT:   name = "xr.slt"; break;
    case XR_SLT_S: name = "xr.slt-s"; break;
    case XR_AND:   name = "xr.and"; break;
    case XR_XOR:   name = "xr.xor"; break;
    case XR_OR:    name = "xr.or"; break;
    case XR_NOR:   name = "xr.nor"; break;
    
    case XR_MUL:   name = "xr.mul"; break;
    case XR_DIV:   name = "xr.div"; break;
    case XR_DIV_S: name = "xr.div-s"; break;
    case XR_MOD:   name = "xr.mod"; break;
    
    case XR_LL:  name = "xr.ll"; break;
    case XR_SC:  name = "xr.sc"; break;
    case XR_MB:  name = "xr.mb"; break;
    case XR_WMB: name = "xr.wmb"; break;
    case XR_BRK: name = "xr.brk"; break;
    case XR_SYS: name = "xr.sys"; break;
    
    case XR_MFCR: name = "xr.mfcr"; break;
    case XR_MTCR: name = "xr.mtcr"; break;
    case XR_HLT:  name = "xr.hlt"; break;
    case XR_RFE:  name = "xr.rfe"; break;

    case XR_P_B:    name = "xr.p.b"; break;
    case XR_P_RET:  name = "xr.p.ret"; break;
    case XR_P_JR:   name = "xr.p.jr"; break;
    case XR_P_MOV:  name = "xr.p.mov"; break;
    case XR_P_LI:   name = "xr.p.li"; break;
    case XR_P_LA:   name = "xr.p.la"; break;
    case XR_P_NOP:  name = "xr.p.nop"; break;
    case XR_P_SHI:  name = "xr.p.shi"; break;
    case XR_P_LOAD8_DIRECT:     name = "xr.p.load8-direct"; break;
    case XR_P_LOAD16_DIRECT:    name = "xr.p.load16-direct"; break;
    case XR_P_LOAD32_DIRECT:    name = "xr.p.load32-direct"; break;
    case XR_P_STORE8_DIRECT:    name = "xr.p.store8-direct"; break;
    case XR_P_STORE16_DIRECT:   name = "xr.p.store16-direct"; break;
    case XR_P_STORE32_DIRECT:   name = "xr.p.store32-direct"; break;
    }

    fe_db_writecstr(db, name);
    fe_db_writecstr(db, " ");

    switch (inst->kind) {
    case XR_J ... XR_JAL:
        if (xr_immsym_is_imm(fe_extra(inst))) {
            fe_db_writef(db, "0x%llx", xr_immsym_imm_val(fe_extra(inst)));
        } else {
            FeSymbol* sym = fe_extra(inst, XrInstImmOrSym)->sym;
            FeCompactStr symname = sym->name;
            fe_db_writef(db, fe_compstr_fmt, fe_compstr_arg(symname));
        }
        break;
    case XR_ADDI ... XR_JALR:
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writef(db, ", %u", fe_extra(inst, XrInstImm)->imm);
        break;
    case XR_LOAD8_IO ... XR_LOAD32_IO:
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writef(db, ", %u", fe_extra(inst, XrInstImm)->imm);
        break;
    case XR_STORE8_IO ... XR_STORE32_IO:
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, inst->inputs[1]);
        fe_db_writef(db, ", %u", fe_extra(inst, XrInstImm)->imm);
        break;
    case XR_STORE8_RO ... XR_STORE32_RO:
        fe__emit_ir_ref(db, f, inst->inputs[0]);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, inst->inputs[1]);
        fe_db_writecstr(db, ", ");
        fe__emit_ir_ref(db, f, inst->inputs[2]);
        break;
    case XR_P_RET:
    case XR_P_NOP:
        break;
    default:
        fe_db_writef(db, "aaa!");
        break;
    }
}

const char* fe_xr_reg_name(u8 regclass, u16 real) {
    const char* name = nullptr;

    switch (real) {
    case XR_GPR_ZERO: name = "zero"; break;
    case XR_GPR_T0: name = "t0"; break;
    case XR_GPR_T1: name = "t1"; break;
    case XR_GPR_T2: name = "t2"; break;
    case XR_GPR_T3: name = "t3"; break;
    case XR_GPR_T4: name = "t4"; break;
    case XR_GPR_T5: name = "t5"; break;
    case XR_GPR_A0: name = "a0"; break;
    case XR_GPR_A1: name = "a1"; break;
    case XR_GPR_A2: name = "a2"; break;
    case XR_GPR_A3: name = "a3"; break;
    case XR_GPR_S0: name = "s0"; break;
    case XR_GPR_S1: name = "s1"; break;
    case XR_GPR_S2: name = "s2"; break;
    case XR_GPR_S3: name = "s3"; break;
    case XR_GPR_S4: name = "s4"; break;
    case XR_GPR_S5: name = "s5"; break;
    case XR_GPR_S6: name = "s6"; break;
    case XR_GPR_S7: name = "s7"; break;
    case XR_GPR_S8: name = "s8"; break;
    case XR_GPR_S9: name = "s9"; break;
    case XR_GPR_S10: name = "s10"; break;
    case XR_GPR_S11: name = "s11"; break;
    case XR_GPR_S12: name = "s12"; break;
    case XR_GPR_S13: name = "s13"; break;
    case XR_GPR_S14: name = "s14"; break;
    case XR_GPR_S15: name = "s15"; break;
    case XR_GPR_S16: name = "s16"; break;
    case XR_GPR_S17: name = "s17"; break;
    case XR_GPR_TP: name = "tp"; break;
    case XR_GPR_SP: name = "sp"; break;
    case XR_GPR_LR: name = "lr"; break;
    default: name = "invalid"; break;
    }

    return name;
}

FeRegStatus fe_xr_reg_status(u8 cconv, u8 regclass, u16 real) {
    FE_ASSERT(cconv == FE_CCONV_ANY || cconv == FE_CCONV_JACKAL);
    FE_ASSERT(regclass == XR_REGCLASS_GPR);

    switch (real) {
    case XR_GPR_T0 ... XR_GPR_T5:
        return FE_REG_CALL_CLOBBERED;
    case XR_GPR_A0 ... XR_GPR_A3:
        return FE_REG_CALL_CLOBBERED;
    case XR_GPR_S0 ... XR_GPR_S17:
        return FE_REG_CALL_PRESERVED;
    default:
        return FE_REG_UNUSABLE;
    }
}
const u16 fe_xr_regclass_lens[XR_REGCLASS__COUNT] = {
    [XR_REGCLASS_GPR] = XR_GPR__COUNT,
};

#define R(x, y) [x - FE__XR_INST_BEGIN ... y - FE__XR_INST_BEGIN]
#define S(x) [x - FE__XR_INST_BEGIN]

const u8 fe_xr_extra_size_table[FE__XR_INST_END - FE__XR_INST_BEGIN] = {
    R(XR_J, XR_JAL) = sizeof(XrInstImmOrSym),
    R(XR_BEQ, XR_BPO) = sizeof(XrInstBranch),
    R(XR_ADDI, XR_JALR) = sizeof(XrInstImm),
    R(XR_SHIFT, XR_SC) = 0,
    R(XR_MB, XR_SYS) = 0,
    R(XR_MFCR, XR_RFE) = sizeof(XrInstImm),
};

#include "../short_traits.h"

const FeTrait fe_xr_trait_table[FE__XR_INST_END - FE__XR_INST_BEGIN] = {
    R(XR_BEQ, XR_BPO) = VOL | TERM,
    S(XR_J) = VOL,
    S(XR_JAL) = VOL,
    S(XR_JALR) = VOL,
    R(XR_STORE8_IO, XR_STORE32_IO) = VOL,
    R(XR_STORE8_RO, XR_STORE32_RO) = VOL,
    R(XR_STORE8_SI, XR_STORE32_SI) = VOL,

    S(XR_P_MOV) = MOV_HINT,
    S(XR_P_RET) = VOL | TERM,
    S(XR_P_B)   = VOL | TERM,
};
