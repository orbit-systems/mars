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
    print_reg(db, vrbuf, inst->inputs[0]);
    if (imm_extra->imm != 0) {
        fe_db_writef(db, "+ %u", imm_extra->imm);
    }
    fe_db_writef(db, "], %u", imm_extra->small);
}

static void print_regular_binop_with_shift(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* name) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);

    fe_db_writecstr(db, name);
    fe_db_writecstr(db, " ");
    print_reg(db, vrbuf, inst);
    fe_db_writecstr(db, ", ");
    print_reg(db, vrbuf, inst->inputs[0]);
    fe_db_writecstr(db, ", ");
    print_reg(db, vrbuf, inst->inputs[1]);
    if (imm_extra->xsh != 0) {
        fe_db_writef(db, "%s %u", 
            shift_kind_op[imm_extra->xsh], 
            imm_extra->shamt
        );
    }
}

static void print_regular_unop(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* name) {
    fe_db_writecstr(db, name);
    fe_db_writecstr(db, " ");
    print_reg(db, vrbuf, inst);
    fe_db_writecstr(db, ", ");
    print_reg(db, vrbuf, inst->inputs[0]);
}


static void print_regular_binop(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* name) {
    fe_db_writecstr(db, name);
    fe_db_writecstr(db, " ");
    print_reg(db, vrbuf, inst);
    fe_db_writecstr(db, ", ");
    print_reg(db, vrbuf, inst->inputs[0]);
    fe_db_writecstr(db, ", ");
    print_reg(db, vrbuf, inst->inputs[1]);
}

static void print_regular_binop_imm(FeDataBuffer* db, FeVRegBuffer* vrbuf, FeInst* inst, const char* name) {
    XrInstImm* imm_extra = fe_extra(inst, XrInstImm);

    fe_db_writecstr(db, name);
    fe_db_writecstr(db, " ");
    print_reg(db, vrbuf, inst);
    fe_db_writecstr(db, ", ");
    print_reg(db, vrbuf, inst->inputs[0]);
    fe_db_writef(db, ", %zu", imm_extra->imm);
}

#include <stdio.h>

static void print_inst(FeDataBuffer* db, FeFunc* f, FeInst* inst) {
    FeVRegBuffer* vrbuf = f->vregs;

    switch (inst->kind) {
        case FE__ROOT:
        case FE_MEM_MERGE:
        case FE_MEM_PHI:
        case FE__MACH_REG:
        // BLOCKED AND IGNORED FUCKERRRRR
        return;
    }

    fe_db_writecstr(db, "    ");
    switch (inst->kind) {
    case XR_LOAD8_IO:   print_load_io(db, vrbuf, inst, "byte"); break;
    case XR_LOAD16_IO:  print_load_io(db, vrbuf, inst, "int"); break;
    case XR_LOAD32_IO:  print_load_io(db, vrbuf, inst, "long"); break;

    case XR_STORE8_IO:  print_store_io(db, vrbuf, inst, "byte"); break;
    case XR_STORE16_IO: print_store_io(db, vrbuf, inst, "int"); break;
    case XR_STORE32_IO: print_store_io(db, vrbuf, inst, "long"); break;

    case XR_LOAD8_RO:   print_load_ro(db, vrbuf, inst, "byte"); break;
    case XR_LOAD16_RO:  print_load_ro(db, vrbuf, inst, "int"); break;
    case XR_LOAD32_RO:  print_load_ro(db, vrbuf, inst, "long"); break;

    case XR_STORE8_RO:  print_store_ro(db, vrbuf, inst, "byte"); break;
    case XR_STORE16_RO: print_store_ro(db, vrbuf, inst, "int"); break;
    case XR_STORE32_RO: print_store_ro(db, vrbuf, inst, "long"); break;

    case XR_STORE8_SI:  print_store_si(db, vrbuf, inst, "byte"); break;
    case XR_STORE16_SI: print_store_si(db, vrbuf, inst, "int"); break;
    case XR_STORE32_SI: print_store_si(db, vrbuf, inst, "long"); break;

    case XR_ADD:   print_regular_binop_with_shift(db, vrbuf, inst, "add"); break;
    case XR_SUB:   print_regular_binop_with_shift(db, vrbuf, inst, "sub"); break;
    case XR_SLT:   print_regular_binop_with_shift(db, vrbuf, inst, "slt"); break;
    case XR_SLT_S: print_regular_binop_with_shift(db, vrbuf, inst, "slt signed"); break;
    case XR_AND:   print_regular_binop_with_shift(db, vrbuf, inst, "and"); break;
    case XR_XOR:   print_regular_binop_with_shift(db, vrbuf, inst, "xor"); break;
    case XR_OR:    print_regular_binop_with_shift(db, vrbuf, inst, "or"); break;
    case XR_NOR:   print_regular_binop_with_shift(db, vrbuf, inst, "nor"); break;

    case XR_ADDI:   print_regular_binop_imm(db, vrbuf, inst, "addi"); break;
    case XR_SUBI:   print_regular_binop_imm(db, vrbuf, inst, "subi"); break;
    case XR_SLTI:   print_regular_binop_imm(db, vrbuf, inst, "slti"); break;
    case XR_SLTI_S: print_regular_binop_imm(db, vrbuf, inst, "slti signed"); break;
    case XR_ORI:    print_regular_binop_imm(db, vrbuf, inst, "ori"); break;
    case XR_LUI:    print_regular_binop_imm(db, vrbuf, inst, "lui"); break;

    case XR_MUL:   print_regular_binop(db, vrbuf, inst, "mul"); break;
    case XR_DIV:   print_regular_binop(db, vrbuf, inst, "div"); break;
    case XR_DIV_S: print_regular_binop(db, vrbuf, inst, "div signed"); break;
    case XR_MOD:   print_regular_binop(db, vrbuf, inst, "mod"); break;

    case XR_LL: print_regular_unop(db, vrbuf, inst, "ll"); break;
    case XR_SC: print_regular_binop(db, vrbuf, inst, "sc"); break;

    case XR_PAUSE: fe_db_writecstr(db, "pause"); break;
    case XR_MB: fe_db_writecstr(db, "mb"); break;
    case XR_WMB: fe_db_writecstr(db, "wmb"); break;
    case XR_BRK: fe_db_writecstr(db, "brk"); break;
    case XR_SYS: fe_db_writecstr(db, "sys"); break;

    case XR_HLT: fe_db_writecstr(db, "hlt"); break;
    case XR_RFE: fe_db_writecstr(db, "rfe"); break;

    case XR_P_RET: fe_db_writecstr(db, "ret"); break;
    case XR_P_NOP: fe_db_writecstr(db, "nop"); break;

    default:
        FE_CRASH("xr: unhandled inst %d when printing asm", inst->kind);
    }

    fe_db_writecstr(db, "\n");
}

void fe_xr_print_text(FeDataBuffer* db, FeModule* m) {
    for (FeSection* section = m->sections.first; section != nullptr; section = section->next) {
        // print section name
        fe_db_writef(db, ".section "fe_compstr_fmt"\n", fe_compstr_arg(section->name));

        // print section flags
        if (section->flags & ~FE_SECTION_WRITEABLE) {
            fe_db_writecstr(db, ".sectionflag ");
            
            if (section->flags & FE_SECTION_WRITEABLE) {
                // sections are writeable by default
            }
            if (section->flags & FE_SECTION_EXECUTABLE) {
                fe_db_writecstr(db, "code, ");
            }
            if (section->flags & FE_SECTION_BLANK) {
                fe_db_writecstr(db, "zeroed, ");
            }
            if (section->flags & FE_SECTION_THREADLOCAL) {
                FE_CRASH("xr17032 does not support THREADLOCAL sections.");
            }
            if (section->flags & FE_SECTION_COMMON) {
                FE_CRASH("xr17032 does not support COMMON sections.");
            }

            // trim trailing comma
            // not necessary, but looks nicer
            db->len -= 2;

            fe_db_writecstr(db, "\n");

        }

        fe_db_writecstr(db, "\n");

        for_funcs(f, m) {
            if (f->sym->section != section) {
                continue;
            }

            // align everything
            fe_db_writef(db, ".align 4\n");

            // emit symbol
            fe_db_writef(db, ""fe_compstr_fmt":\n", fe_compstr_arg(f->sym->name));
            fe_db_writef(db, ".global "fe_compstr_fmt"\n", fe_compstr_arg(f->sym->name));

            // emit instruction by instruction
            for_blocks(b, f) {
                for_inst(i, b) {
                    print_inst(db, f, i);
                }
            }
            fe_db_writef(db, "\n");
        }
    }
}
