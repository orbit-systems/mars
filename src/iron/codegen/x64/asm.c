#include "x64.h"

enum {
    GPR_64,
    GPR_32,
    GPR_16,
    GPR_8,

    _GPR_LEVEL_MAX
};

static const char* gpr_names[_FE_X64_GPR_MAX][_GPR_LEVEL_MAX] = {
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
            fe_db_write_cstring(db, gpr_names[reg.real][bits], false);
            break;
        default:
            fe_db_write_cstring(db, "?unknown_regclass", false);
            break;
        }
    }
}

void fe_x64_assemble_text(FeMachBuffer* buf) {
    
}