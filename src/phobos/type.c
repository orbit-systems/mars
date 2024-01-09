#include "orbit.h"
#include "lexer.h"
#include "error.h"
#include "arena.h"
#include "type.h"

size_t mt_type_size[] = {
    0,
#define TYPE(ident, identstr, structdef) sizeof(mtype_##ident),
    TYPE_NODES
#undef TYPE
    0
};

char* mt_type_str[] = {
    "invalid",
#define TYPE(ident, identstr, structdef) identstr,
    TYPE_NODES
#undef TYPE
    "COUNT",
};

// allocate and zero a new AST node with an arena_list
mars_type new_type_node(arena_list* restrict al, mt_type type) {
    mars_type node;
    void* node_ptr = arena_list_alloc(al, mt_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("new_type_node() could not allocate type node of type '%s' with size %d", mt_type_str[type], mt_type_size[type]);
    }
    memset(node_ptr, 0, mt_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}