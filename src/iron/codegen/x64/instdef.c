#include "x64.h"

FeMachBuffer fe_x64_codegen(FeModule* mod);
void fe_x64_emit_text(FeDataBuffer* db, FeMachBuffer* machbuf);

#define INSTDEF(code) \
    }                 \
    , [code] = {.template_index = code,

const FeMachInstTemplate fe_x64_inst_templates[_FE_X64_INST_COUNT] = {
    {
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
        INSTDEF(FE_X64_INST_RET)
            .regs_len = 0,
        .defs = 0b00000000,
        .uses = 0b00000000,
        .imms_len = 0,
    }
};

static const FeArchRegclass regclasses[] = {
    [FE_X64_REGCLASS_UNKNOWN] = {},
    [FE_X64_REGCLASS_GPR] = {
        .id = FE_X64_REGCLASS_GPR,
        .len = _FE_X64_GPR_COUNT,
    },
};

const FeArchInfo fe_arch_x64 = {
    .name = "x64",

    .cg = fe_x64_codegen,
    .emit_text = fe_x64_emit_text,

    .native_int = FE_TYPE_I64,
    .native_float = FE_TYPE_F64,

    .regclasses = {
        .at = regclasses,
        .len = sizeof(regclasses) / sizeof(regclasses[0]),
    },

    .inst_templates = {
        .at = fe_x64_inst_templates,
        .len = sizeof(fe_x64_inst_templates) / sizeof(fe_x64_inst_templates[0]),
    },
};