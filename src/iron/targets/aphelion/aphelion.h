#pragma once

#include "iron.h"
#include "targets/asmprinter.h"

enum {
    APHEL_REGCLASS_ZERO,
    APHEL_REGCLASS_GPR,
    APHEL_REGCLASS_LEN,
};

extern const FeArchRegisterInfo aphelion_regclass_zero_regs[];
enum {
    APHEL_ZERO_RZ,
    APHEL_ZERO_LEN,
};

extern const FeArchRegisterInfo aphelion_regclass_gpr_regs[];
enum {
    APHEL_GPR_RA,
    APHEL_GPR_RB,
    APHEL_GPR_RC,
    APHEL_GPR_RD,
    APHEL_GPR_RE,
    APHEL_GPR_RF,
    APHEL_GPR_RG,
    APHEL_GPR_RH,
    APHEL_GPR_RI,
    APHEL_GPR_RJ,
    APHEL_GPR_RK,
    APHEL_GPR_LEN,
};

extern const FeArchInstInfo aphelion_instructions[];
enum {
    APHEL_INST_NOP,
    APHEL_INST_MOV,

    APHEL_INST_ADDR,
    APHEL_INST_ADDI,

    APHEL_INST_IMULR,
    APHEL_INST_IMULI,

    APHEL_INST_RET,

    APHEL_INST_LEN,
};

extern const FeArchInfo aphelion_target_info;

extern FePass pass_aphelion_codegen;
extern FePass pass_aphelion_movopt;