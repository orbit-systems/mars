#pragma once
#define ATLAS_PASS_H

#include "ir.h"
#include "target.h"

enum {
    PASS_IR_TO_IR,
    PASS_IR_TO_ASM,
    PASS_ASM_TO_ASM,
};

typedef struct AtlasPass {
    string name;
    union {
        void* callback;

        void (* ir2ir_callback   )(AIR_Module*);
        void (* asm2asm_callback )(AsmModule*);
        void (* ir2asm_callback  )(AIR_Module*, AsmModule*);
    };
    u8 kind;

    // make sure CFG info is up-to-date before this pass runs
    bool requires_cfg;
    
    // mark the CFG information as out-of-date
    bool modifies_cfg;
} AtlasPass;

da_typedef(AtlasPass);