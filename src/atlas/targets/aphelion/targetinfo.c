#include "aphelion.h"
#include "atlas.h"
#include "ir.h"

const TargetRegisterInfo aphelion_regclass_zero_regs[] = {
    [APHEL_ZERO_RZ] = { .name = constr("rz")},
};

const TargetRegisterInfo aphelion_regclass_gpr_regs[] = {
    [APHEL_GPR_RA] = { .name = constr("ra") },
    [APHEL_GPR_RB] = { .name = constr("rb") },
    [APHEL_GPR_RC] = { .name = constr("rc") },
    [APHEL_GPR_RD] = { .name = constr("rd") },
    [APHEL_GPR_RE] = { .name = constr("re") },
    [APHEL_GPR_RF] = { .name = constr("rf") },
    [APHEL_GPR_RG] = { .name = constr("rg") },
    [APHEL_GPR_RH] = { .name = constr("rh") },
    [APHEL_GPR_RI] = { .name = constr("ri") },
    [APHEL_GPR_RJ] = { .name = constr("rj") },
    [APHEL_GPR_RK] = { .name = constr("rk") },
};

const TargetInstInfo aphelion_instructions[] = {
    [APHEL_INST_NOP] = {
        .asm_string = constr("nop"),
    },
    [APHEL_INST_MOV] = {
        .asm_string = constr("mov  {out 0}, {in 0}"),
        .num_ins = 1,
        .num_outs = 1,
    },
    [APHEL_INST_ADDR] = {
        .asm_string = constr("addr {out 0}, {in 0}, {in 1}"),
        .num_ins = 2,
        .num_outs = 1,
    },
    [APHEL_INST_ADDI] = {
        .asm_string = constr("addi {out 0}, {in 0}, {int 0}"),
        .num_ins = 1,
        .num_imms = 1,
        .num_outs = 1,
    },
    [APHEL_INST_IMULR] = {
        .asm_string = constr("imulr {out 0}, {in 0}, {in 1}"),
        .num_ins = 2,
        .num_outs = 1,
    },
    [APHEL_INST_IMULI] = {
        .asm_string = constr("imuli {out 0}, {in 0}, {int 0}"),
        .num_ins = 1,
        .num_imms = 1,
        .num_outs = 1,
    },
    [APHEL_INST_RET] = {
        .asm_string = constr("ret"),
    },
};

const TargetFormatInfo aphelion_format_info = {
    .i64 = constr("d64 {}"),
    .i32 = constr("d32 {}"),
    .i16 = constr("d16 {}"),
    .i8  = constr("d8 {}"),
    
    .u64 = constr("d64 {}"),
    .u32 = constr("d32 {}"),
    .u16 = constr("d16 {}"),
    .u8  = constr("d8 {}"),

    .zero = constr("zero {}"),

    .string = NULL_STR, // just use a u8 sequence for now

    .align = constr("align {}"),

    .label = constr("{}:"),
    .block_label = constr(".{}:"),

    .bind_symbol_global = constr("global {}"),
    .bind_symbol_local  = constr("local {}"),

    .begin_code_section = constr(""), // nothing for now
    .begin_data_section = constr(""), // nothing for now
    .begin_rodata_section = constr(""), // nothing for now
    .begin_bss_section = constr(""), // nothing for now
};

// all the information that makes up the aphelion target.
const TargetInfo aphelion_target_info = {

    // name displayed
    .name = "aphelion",

    // register classes
    .regclasses = (TargetRegisterClass[]){
        [APHEL_REGCLASS_ZERO] = {
            .regs = aphelion_regclass_zero_regs,
            .regs_len = APHEL_ZERO_LEN,
        },
        [APHEL_REGCLASS_GPR] = {
            .regs = aphelion_regclass_gpr_regs,
            .regs_len = APHEL_GPR_LEN,
        },
    },
    .regclasses_len = APHEL_REGCLASS_LEN,

    // instruction templates
    .insts = aphelion_instructions,
    .insts_len = APHEL_INST_LEN,
    .inst_align = 4,

    .format_info = &aphelion_format_info,
};