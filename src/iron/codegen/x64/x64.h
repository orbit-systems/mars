#include "iron/codegen/mach.h"

enum {
    FE_X64_REGCLASS_UNKNOWN = 0,
    FE_X64_REGCLASS_GPR,
};

enum {
    FE_X64_GPR_UNKNOWN = 0,

    FE_X64_GPR_RAX, // all variants of a register are treated the same.
    FE_X64_GPR_RBX,
    FE_X64_GPR_RCX,
    FE_X64_GPR_RDX,
    FE_X64_GPR_RSI,
    FE_X64_GPR_RDI,
    FE_X64_GPR_RBP,
    FE_X64_GPR_RSP,
    FE_X64_GPR_R8,
    FE_X64_GPR_R9,
    FE_X64_GPR_R10,
    FE_X64_GPR_R11,
    FE_X64_GPR_R12,
    FE_X64_GPR_R13,
    FE_X64_GPR_R14,
    FE_X64_GPR_R15,

    _FE_X64_GPR_MAX,
};

enum {
    FE_X64_INST_UNKNOWN = 0,

    FE_X64_INST_MOV_RR_64,   // mov def, use
    FE_X64_INST_ADD_RR_64,   // add def/use, use
    FE_X64_INST_LEA_RR_64,   // lea def, [use + use]

    _FE_X64_INST_MAX,
};

extern const FeMachInstTemplate fe_x64_inst_templates[_FE_X64_INST_MAX];

FeMachBuffer fe_x64_codegen(FeModule* mod);
void fe_x64_emit_text(FeDataBuffer* db, FeMachBuffer* machbuf);

// x64-specific instructions
enum {
    _FE_INST_X64_START = _FE_INST_ARCH_SPECIFIC_START,
};