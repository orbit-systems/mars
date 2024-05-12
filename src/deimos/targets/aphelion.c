#include "target.h"

#include "aphelion.h"

const TargetRegisterInfo aphelion_regclass_gpr_regs[] = {
    [APHEL_GPR_RZ] = { .name = constr("rz") },
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

const TargetInstruction aphelion_instructions[] = {
    [APHEL_INST_NOP] = {
        .asm_string = constr("nop"),
    },
    [APHEL_INST_MOV] = {
        .asm_string = constr("mov {out 0}, {in 0}"),
        .num_ins = 1,
        .num_outs = 1,
        .is_mov = true,
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
    [APHEL_INST_RET] = {
        .asm_string = constr("ret"),
    },
};

// all the information that makes up the aphelion target.
const TargetInfo aphelion_target_info = (TargetInfo){

    // name displayed
    .name = "aphelion",

    // register classes
    .regclasses = (TargetRegisterClass[]){
        [APHEL_REGCLASS_GPR] = {
            .regs = aphelion_regclass_gpr_regs,
            .regs_len = APHEL_GPR_LEN
        },
    },
    .regclasses_len = APHEL_REGCLASS_LEN,

    // instruction templates
    .insts = aphelion_instructions,
    .insts_len = APHEL_INST_LEN,

};