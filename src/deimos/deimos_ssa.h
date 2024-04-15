#pragma once
#define DEIMOS_SSA_H

#include "orbit.h"
#include "../phobos/lex.h"
#include "../arena.h"
#include "../term.h"
#include "../phobos/ast.h"

typedef struct {
} ssa_base;

// define all the DSSA node macros
#define SSA_NODES \
    SSA_TYPE(pruned, "pruned SSA node", { \
        ssa_base base; \
    }) \

// generate the enum tags for the DSSA tagged union
typedef u16 ssa_type; enum {
    ssa_type_invalid,
#define SSA_TYPE(ident, identstr, structdef) ssa_type_##ident,
    SSA_NODES
#undef SSA_TYPE
    ssa_type_COUNT,
};

// generate tagged union DSSA type
typedef struct SSA {
    union {
        void* rawptr;
        ssa_base * base;
#define SSA_TYPE(ident, identstr, structdef) struct ssa_##ident * as_##ident;
        SSA_NODES
#undef SSA_TYPE
    };
    ssa_type type;
} SSA;

da_typedef(SSA);

// generate DSSA node typedefs
#define SSA_TYPE(ident, identstr, structdef) typedef struct ssa_##ident structdef ssa_##ident;
    SSA_NODES
#undef SSA_TYPE

#define NULL_SSA ((SSA){0})
#define is_null_SSA(node) ((node).type == 0 || (node).rawptr == NULL)

extern char* ssa_type_str[];
extern size_t ssa_type_size[];

SSA new_dssa_entry(arena* restrict alloca, ssa_type type);