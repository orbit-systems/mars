#include "x64.h"

#define INSTDEF(code) }, [code] = { .template_index = code,

const FeMachInstTemplate fe_x64_inst_templates[_FE_X64_INST_MAX] = {{
    INSTDEF(FE_X64_INST_MOV_RR_64)
        .regs_len = 2,
        .defs = 0b00000001, // first register
        .uses = 0b00000010, // second register
        .imms_len = 0,
    INSTDEF(FE_X64_INST_ADD_RR_64)
        .regs_len = 2,
        .defs = 0b00000001,
        .uses = 0b00000011, // first register is a def and a use
        .imms_len = 0,
    INSTDEF(FE_X64_INST_LEA_RR_64)
        .regs_len = 3,
        .defs = 0b00000001,
        .uses = 0b00000110,
        .imms_len = 0,
}};

const FeInstArchInfo fe_x64_inst_arch_info[] = {
    
};