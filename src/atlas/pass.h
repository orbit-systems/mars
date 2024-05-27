#pragma once
#define ATLAS_PASS_H

#include "ir.h"

enum {
    PASS_IR_TO_IR,
    PASS_IR_TO_IR,
    PASS_MI_TO_MI,
};

typedef struct {
    string name;
    union {
        void* callback;
        AIR_Module* (*air_callback)(AIR_Module*);
        //MInst->MInst
    };
    u8 type;
} Pass;

da_typedef(Pass);
