#pragma once
#define DEIMOS_MIC_H

// MIL - mars intermediate language
// big shit real shit idk lmao

#include "orbit.h"
#include "arena.h"

typedef u8 mil_inst_type; enum {
    inst_local_load,
    inst_local_store,
    inst_load,
    inst_store,

    inst_add,
    inst_sub,
    inst_imul,
    inst_umul,
    inst_idiv,
    inst_udiv,

    // TODO
};

typedef struct live_range {
    u32 definition;
    u32 last_use;
} live_range;

typedef struct mil_entity {
    string name;
    live_range range;
} mil_entity;

da_typedef(mil_entity);

// basic block
typedef struct mil_bb {
    da(mil_entity) entities;
} mil_bb;