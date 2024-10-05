#include "x64.h"

enum {
    GPR_64,
    GPR_32,
    GPR_16,
    GPR_8,

    _GPR_LEVEL_MAX
};

static const char* gpr_names[_FE_X64_GPR_COUNT][_GPR_LEVEL_MAX] = {
    [FE_X64_GPR_RAX] = {"rax", "eax",  "ax",   "al"},
    [FE_X64_GPR_RBX] = {"rbx", "ebx",  "bx",   "bl"},
    [FE_X64_GPR_RCX] = {"rcx", "ecx",  "cx",   "cl"},
    [FE_X64_GPR_RDX] = {"rdx", "edx",  "dx",   "dl"},
    [FE_X64_GPR_RSI] = {"rsi", "esi",  "si",   "sl"},
    [FE_X64_GPR_RDI] = {"rdi", "edi",  "di",   "dl"},
    [FE_X64_GPR_RBP] = {"rbp", "ebp",  "bp",   "bpl"},
    [FE_X64_GPR_RSP] = {"rsp", "esp",  "sp",   "spl"},
    [FE_X64_GPR_R8]  = {"r8",  "r8d",  "r8w",  "r8b"},
    [FE_X64_GPR_R9]  = {"r9",  "r9d",  "r9w",  "r9b"},
    [FE_X64_GPR_R10] = {"r10", "r10d", "r10w", "r10b"},
    [FE_X64_GPR_R11] = {"r11", "r11d", "r11w", "r11b"},
    [FE_X64_GPR_R12] = {"r12", "r12d", "r12w", "r12b"},
    [FE_X64_GPR_R13] = {"r13", "r13d", "r13w", "r13b"},
    [FE_X64_GPR_R14] = {"r14", "r14d", "r14w", "r14b"},
    [FE_X64_GPR_R15] = {"r15", "r15d", "r15w", "r15b"},
};

static void emit_register(FeDataBuffer* db, FeMachBuffer* buf, u32 vreg, u8 bits) {
    if (vreg == 0) {
        fe_db_write_cstring(db, "---");
        return;
    }
    FeMachVReg reg = buf->vregs.at[vreg];
    if (reg.real == 0) { // still virtual
        fe_db_write_format(db, "v%u", vreg);
        switch (bits) {
        case GPR_64: fe_db_write_8(db, 'q'); break;
        case GPR_32: fe_db_write_8(db, 'd'); break;
        case GPR_16: fe_db_write_8(db, 'w'); break;
        case GPR_8:  fe_db_write_8(db, 'b'); break;
        default:     fe_db_write_8(db, '?'); break;
        }
    } else {
        switch (reg.class) {
        case FE_X64_REGCLASS_GPR:
            fe_db_write_cstring(db, gpr_names[reg.real][bits]);
            break;
        default:
            fe_db_write_format(db, "?regclass_%d", reg.class);
            break;
        }
    }
}

#define vr_index(reglist, index) buf->vreg_lists.at[reglist + index]

static void emit_inst(FeDataBuffer* db, FeMachBuffer* buf, FeMach* m) {
    FeMachInst* i = (FeMachInst*) m;
    const FeMachInstTemplate* templ = &buf->target.inst_templates[i->template];
         
    switch (i->template) {
    case FE_X64_INST_ADD_RR_64:
        // add r1, r2
        fe_db_write_cstring(db, "add ");
        emit_register(db, buf, vr_index(i->regs, 0), GPR_64);
        fe_db_write_cstring(db, ", ");
        emit_register(db, buf, vr_index(i->regs, 1), GPR_64);
        break;
    case FE_X64_INST_MOV_RR_64:
        // mov r1, r2
        fe_db_write_cstring(db, "mov ");
        emit_register(db, buf, vr_index(i->regs, 0), GPR_64);
        fe_db_write_cstring(db, ", ");
        emit_register(db, buf, vr_index(i->regs, 1), GPR_64);
        break;
    case FE_X64_INST_LEA_RR_64:
        // lea r1, [r2 + r3]
        fe_db_write_cstring(db, "lea");
        emit_register(db, buf, vr_index(i->regs, 0), GPR_64);
        fe_db_write_cstring(db, ", [");
        emit_register(db, buf, vr_index(i->regs, 1), GPR_64);
        fe_db_write_cstring(db, " + ");
        emit_register(db, buf, vr_index(i->regs, 2), GPR_64);
        fe_db_write_cstring(db, "]");
        break;
    case FE_X64_INST_RET:
        fe_db_write_cstring(db, "ret");
        break;
    }
}

void fe_x64_emit_text(FeDataBuffer* db, FeMachBuffer* machbuf) {
    for_range(i, 0, machbuf->buf.len) {
        FeMach* m = machbuf->buf.at[i];
        if (m->kind == 0) continue;
        fe_db_write_format(db, "% 3d |   ", i);
        switch (m->kind) {
        case FE_MACH_CFG_BRANCH:
            fe_db_write_cstring(db, "; (IRON) cfg branch\n");
            break;
        case FE_MACH_CFG_JUMP:
            fe_db_write_cstring(db, "; (IRON) cfg jump\n");
            break;
        case FE_MACH_CFG_TARGET:
            fe_db_write_cstring(db, "; (IRON) cfg target\n");
            break;
        case FE_MACH_REGALLOC_BEGIN:
            fe_db_write_cstring(db, "; (IRON) regalloc begin\n");
            break;
        case FE_MACH_REGALLOC_END:
            fe_db_write_cstring(db, "; (IRON) regalloc end\n");
            break;
        case FE_MACH_LIFETIME_BEGIN:
            FeMachLifetimePoint* ltp = (FeMachLifetimePoint*) m;
            fe_db_write_cstring(db, "; (IRON) lifetime begin ");
            emit_register(db, machbuf, ltp->vreg, GPR_64);
            fe_db_write_cstring(db, "\n");
            break;
        case FE_MACH_LIFETIME_END:
            ltp = (FeMachLifetimePoint*) m;
            fe_db_write_cstring(db, "; (IRON) lifetime end ");
            emit_register(db, machbuf, ltp->vreg, GPR_64);
            fe_db_write_8(db, '\n');
            break;
        case FE_MACH_INST:
            fe_db_write_cstring(db, "   ");
            emit_inst(db, machbuf, m);
            fe_db_write_8(db, '\n');
            break;
        case FE_MACH_LABEL_LOCAL:
            fe_db_write_8(db, '.');
            FeMachLocalLabel* local = (FeMachLocalLabel*) m;
            fe_db_write_string(db, local->name);
            fe_db_write_cstring(db, ":\n");
            break;
        case FE_MACH_LABEL_GLOBAL:
            FeMachGlobalLabel* label = (FeMachGlobalLabel*) m;
            FeMachSymbol sym = machbuf->symtab.at[label->symbol_index];
            fe_db_write_bytes(db, sym.name, sym.name_len);
            fe_db_write_cstring(db, ":\n");
            break;
        }
    }

    
}