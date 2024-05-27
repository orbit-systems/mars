#pragma once
#define ATLAS_H

typedef struct AtlasModule AtlasModule;

#include "orbit.h"
#include "alloc.h"
#include "ir.h"
#include "pass.h"
#include "target.h"

typedef struct AtlasModule {
    string name;

    AIR_Module ir_module;
    AsmModule asm_module;

} AtlasModule;

AtlasModule* atlas_new_module(string name);

char* random_string(int len);