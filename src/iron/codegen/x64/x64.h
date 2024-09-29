#include "iron/codegen/mach.h"

enum {
    FE_X64_REGCLASS_UNKNOWN = 0,
    FE_X64_REGCLASS_GPR,
};

enum {
    FE_X64_GPR_UNKNOWN = 0,
    FE_X64_GPR_RAX,
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
};

enum {
    FE_X64_INST_UNKNOWN,
    FE_X64_INST_MOV_RR_64,   // mov r1, r2
    FE_X64_INST_LEA_RR_64,   // lea r1, [r2, r3]

    _FE_X64_INST_MAX,
};

extern const FeMachInstTemplate fe_x64_inst_templates[_FE_X64_INST_MAX];