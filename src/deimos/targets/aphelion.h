#pragma once
#define DEIMOS_TARGET_APHELION_H

#include "target.h"

// aphelion only has one register class, the GPRs
enum {
    APHEL_REGCLASS_GPR,
    APHEL_REGCLASS_LEN,
};


extern const TargetRegisterInfo aphelion_regclass_gpr_regs[];
enum {
    APHEL_GPR_RZ,
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

extern const TargetInstruction aphelion_instruction_templates[];
enum {
    APHEL_INST_NOP,
    APHEL_INST_MOV,

    APHEL_INST_ADDR,
    APHEL_INST_ADDI,

    APHEL_INST_RET,

    APHEL_INST_LEN,
};

extern const TargetInfo aphelion_target_info;