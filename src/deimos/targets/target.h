#pragma once
#define DEIMOS_TARGET_H

typedef enum {
    OPERAND_REGISTER,
    OPERAND_IMMEDIATE,
} MI_Operand_Type;

typedef struct {
    MI_Operand_Type type;
    int imm_len;
} MI_Operand_Template;

typedef struct {
    MI_Operand_Template* template;
    union {
        int reg_idx;
        u64 value;
    }
} MI_Operand;

da_typedef(MI_Operand);

typedef struct {
    char* name;
    char* argstr;
    char* mnemonic;
    da(MI_Operand_Template) operands;
} MInst_Template;

typedef struct {
    MInst_Template* template;
    da(MI_Operand) operand_values;
} MInst;

typedef MInst* (*pass_template)(MInst*); //creates a typedef for pass_template of type MI* (*)(MI*).

typedef struct {
    char* name;
    MInst* (*IR_to_MI)(IR_Module*); /*conversion from IR to target-specific MInst */
    da(MInst_Template) templates;
} Target;