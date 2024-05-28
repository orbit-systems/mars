#pragma once
#define ATLAS_H

#include "orbit.h"
#include "alloc.h"

typedef struct AtlasModule AtlasModule;

#include "ir.h"
#include "pass.h"
#include "target.h"

#include "targets/aphelion/aphelion.h"

typedef struct AtlasModule {
    string name;

    AIR_Module* ir_module;
    AsmModule* asm_module;

    struct {
        AtlasPass** at;
        size_t len;
        size_t cap;

        bool cfg_up_to_date;
    } pass_queue;

} AtlasModule;

AtlasModule* atlas_new_module(string name, TargetInfo* target);

char* random_string(int len);