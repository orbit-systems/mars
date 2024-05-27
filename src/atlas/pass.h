#pragma once
#define ATLAS_PASS_H

#include "ir.h"

typedef enum {
    PASS_AIR_TO_IR,
    PASS_MI_TO_MI,
} pass_type;

typedef struct {
    char* name;
    union {
        void* callback;
        AIR_Module* (*air_callback)(AIR_Module*);
        //MInst->MInst
    };
    pass_type type;
} Pass;

void run_passes(AIR_Module* current_program);
void register_passes();

void add_pass(char* name, void* callback, pass_type type);

da_typedef(Pass);

extern da(Pass) atlas_passes;