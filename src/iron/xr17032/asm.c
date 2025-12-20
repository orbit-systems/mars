#include "iron/iron.h"
#include "xr.h"

/// Write a module `m` to `db` as assembly text.
void fe_xr_emit_asm(FeDataBuffer* db, FeModule* m) {

}

static void print_reg(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst) {
    const FeVirtualReg* vreg = &vrbuf->at[inst->vr_def];
    const char* reg_name = fe_xr_reg_name(vreg->class, vreg->real);
    fe_db_writecstr(db, reg_name);
}

static void print_load_io(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* specifier) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);
    
    fe_db_writecstr(db, "mov ");
    print_reg(db, vrbuf, inst);
    fe_db_writef(db, ", %s [", specifier);
    print_reg(db, vrbuf, inst->inputs[0]);
    if (imm_extra->imm != 0) {
        fe_db_writef(db, "+ %u", imm_extra->imm);
    }
    fe_db_writecstr(db, "]");
}

static void print_store_io(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* specifier) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);
    
    fe_db_writef(db, "mov %s [", specifier);
    print_reg(db, vrbuf, inst->inputs[1]);
    if (imm_extra->imm != 0) {
        fe_db_writef(db, "+ %u", imm_extra->imm);
    }
    fe_db_writecstr(db, "], ");
    print_reg(db, vrbuf, inst->inputs[0]);
}

static const char* shift_kind_op[4] = {
    [XR_SHIFT_LSH] = "LSH",
    [XR_SHIFT_RSH] = "RSH",
    [XR_SHIFT_ASH] = "ASH",
    [XR_SHIFT_ROR] = "ROR",
};

static void print_load_ro(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* specifier) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);
    
    fe_db_writecstr(db, "mov ");
    print_reg(db, vrbuf, inst);
    fe_db_writef(db, ", %s [", specifier);
    print_reg(db, vrbuf, inst->inputs[0]);
    fe_db_writecstr(db, " + ");
    print_reg(db, vrbuf, inst->inputs[1]);
    if (imm_extra->xsh != 0) {
        fe_db_writef(db, "%s %u", 
            shift_kind_op[imm_extra->xsh], 
            imm_extra->shamt
        );
    }
    fe_db_writecstr(db, "]");
}

static void print_store_ro(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* specifier) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);
    
    fe_db_writecstr(db, "mov ");
    fe_db_writef(db, "%s [", specifier);
    print_reg(db, vrbuf, inst->inputs[0]);
    fe_db_writecstr(db, " + ");
    print_reg(db, vrbuf, inst->inputs[1]);
    if (imm_extra->xsh != 0) {
        fe_db_writef(db, "%s %u", 
            shift_kind_op[imm_extra->xsh], 
            imm_extra->shamt
        );
    }
    fe_db_writecstr(db, "], ");
    print_reg(db, vrbuf, inst->inputs[2]);
}


static void print_store_si(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* specifier) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);
    
    fe_db_writef(db, "mov %s [", specifier);
    print_reg(db, vrbuf, inst->inputs[1]);
    if (imm_extra->imm != 0) {
        fe_db_writef(db, "+ %u", imm_extra->imm);
    }
    fe_db_writef(db, "], %u", imm_extra->imm);
}


static void print_inst(FeDataBuffer* db, FeFunc* f, FeInst* inst) {
    FeVRegBuffer* vrbuf = f->vregs;

    switch (inst->kind) {
    case FE__ROOT:
    case FE_MEM_MERGE:
    case FE_MEM_PHI:
        // BLOCKED AND IGNORED FUCKERRRRR
        return;
    case XR_LOAD8_IO:
        print_load_io(db, vrbuf, inst, "byte");
        break;
    case XR_LOAD16_IO:
        print_load_io(db, vrbuf, inst, "int");
        break;
    case XR_LOAD32_IO:
        print_load_io(db, vrbuf, inst, "long");
        break;
    case XR_STORE8_IO:
        print_store_io(db, vrbuf, inst, "byte");
        break;
    case XR_STORE16_IO:
        print_store_io(db, vrbuf, inst, "int");
        break;
    case XR_STORE32_IO:
        print_store_io(db, vrbuf, inst, "long");
        break;
    case XR_LOAD8_RO:
        print_load_ro(db, vrbuf, inst, "byte");
        break;
    case XR_LOAD16_RO:
        print_load_ro(db, vrbuf, inst, "int");
        break;
    case XR_LOAD32_RO:
        print_load_ro(db, vrbuf, inst, "long");
        break;
    case XR_STORE8_RO:
        print_store_ro(db, vrbuf, inst, "byte");
        break;
    case XR_STORE16_RO:
        print_store_ro(db, vrbuf, inst, "int");
        break;
    case XR_STORE32_RO:
        print_store_ro(db, vrbuf, inst, "long");
        break;
    case XR_STORE8_SI:
        print_store_si(db, vrbuf, inst, "byte");
        break;
    case XR_STORE16_SI:
        print_store_si(db, vrbuf, inst, "int");
        break;
    case XR_STORE32_SI:
        print_store_si(db, vrbuf, inst, "long");
        break;
    default:
        FE_CRASH("xr: unhandled inst %d when printing asm", inst->kind);
    }
}
