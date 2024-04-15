#include "deimos_ssa.h"

size_t ssa_type_size[] = {
    0,
#define SSA_TYPE(ident, identstr, structdef) sizeof(ssa_##ident),
    SSA_NODES
#undef SSA_TYPE
    0
};

char* ssa_type_str[] = {
    "invalid",
#define SSA_TYPE(ident, identstr, structdef) identstr,
    SSA_NODES
#undef SSA_TYPE
    "COUNT",
};

// allocate and zero a new DSSA node with an arena
SSA new_ssa_entry(arena* restrict alloca, ssa_type type) {
    SSA node;
    void* node_ptr = arena_alloc(alloca, ssa_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_ssa_entry() could not allocate deimos SSA entry of type '%s' with size %d", ssa_type_str[type], ssa_type_size[type]);
    }
    memset(node_ptr, 0, ssa_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}