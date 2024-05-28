#pragma once
#define ATLAS_H

typedef struct AtlasModule AtlasModule;

#include "orbit.h"
#include "alloc.h"

#include "atlas/ir.h"
#include "atlas/pass.h"
#include "atlas/targets/target.h"
#include "atlas/targets/aphelion/aphelion.h"

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