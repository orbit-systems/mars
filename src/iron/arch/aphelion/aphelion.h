#pragma once

#include "iron/iron.h"

typedef struct FeAphelionArchConfig {
    bool ext_f; // extension f - floating point operations
} FeAphelionArchConfig;

enum {
    APHEL_REGCLASS_ZERO,
    APHEL_REGCLASS_GPR,
    
    _APHEL_REGCLASS_LEN,
};

extern const FeArchRegisterInfo aphelion_regclass_zero_regs[];
enum {
    APHEL_ZERO_RZ,

    _APHEL_ZERO_LEN,
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
    APHEL_GPR_RL,
    APHEL_GPR_RM,

    _APHEL_GPR_LEN,
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

extern const FeArchInfo aphelion_arch_info;