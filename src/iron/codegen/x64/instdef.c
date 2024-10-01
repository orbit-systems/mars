#include "x64.h"

#define INSTDEF(code) }, [code] = { .inst = code,

const FeMachInstTemplate fe_x64_inst_templates[_FE_X64_INST_MAX] = {{
    INSTDEF(FE_X64_INST_MOV_RR_64)
        .uses_len = 1,
        .defs_len = 1,
        .imms_len = 0,
    INSTDEF(FE_X64_INST_ADD_RR_64)
        .uses_len = 2, // first register is a def and use
        .defs_len = 1,
        .imms_len = 0,
    INSTDEF(FE_X64_INST_LEA_RR_64)
        .uses_len = 2,
        .defs_len = 1,
        .imms_len = 0,
}};