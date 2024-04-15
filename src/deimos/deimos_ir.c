#include "deimos_ir.h"

size_t ir_type_size[] = {
    0,
#define IR_TYPE(ident, identstr, structdef) sizeof(ir_##ident),
    IR_NODES
#undef IR_TYPE
    0
};

char* ir_type_str[] = {
    "invalid",
#define IR_TYPE(ident, identstr, structdef) identstr,
    IR_NODES
#undef IR_TYPE
    "COUNT",
};

// allocate and zero a new DIR node with an arena
IR new_ir_entry(arena* restrict alloca, ir_type type) {
    IR node;
    void* node_ptr = arena_alloc(alloca, ir_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_ir_entry() could not allocate deimos IR entry of type '%s' with size %d", ir_type_str[type], ir_type_size[type]);
    }
    memset(node_ptr, 0, ir_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}