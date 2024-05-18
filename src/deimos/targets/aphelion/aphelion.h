#pragma once
#define TARGET_APHELION_H

#include "../target.h"
#include "../asmprinter.h"

AsmModule* aphelion_translate_module(IR_Module* irmod);

enum {
    APHEL_REGCLASS_ZERO,
    APHEL_REGCLASS_GPR,
    APHEL_REGCLASS_LEN,
};

extern const TargetRegisterInfo aphelion_regclass_zero_regs[];
enum {
    APHEL_ZERO_RZ,
    APHEL_ZERO_LEN,
};

extern const TargetRegisterInfo aphelion_regclass_gpr_regs[];
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

extern const TargetInstInfo aphelion_instruction_templates[];
enum {
    APHEL_INST_NOP,
    APHEL_INST_MOV,

    APHEL_INST_ADDR,
    APHEL_INST_ADDI,

    APHEL_INST_RET,

    APHEL_INST_LEN,
};

extern const TargetInfo aphelion_target_info;