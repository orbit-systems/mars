#include <stdalign.h>

#include "orbit.h"
#include "lexer.h"
#include "error.h"
#include "ast.h"
#include "arena.h"

size_t ast_type_size[] = {
    0,
#define AST_TYPE(ident, identstr, structdef) sizeof(ast_##ident),
    AST_NODES
#undef AST_TYPE
    0
};

char* ast_type_str[] = {
    "invalid",
#define AST_TYPE(ident, identstr, structdef) identstr,
    AST_NODES
#undef AST_TYPE
    "COUNT",
};



// allocate and zero a new AST node with an arena_list
AST new_ast_node(arena_list* restrict al, ast_type type) {
    AST node;
    void* node_ptr = arena_list_alloc(al, ast_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_ast_node() could not allocate AST node of type '%s' with size %d", ast_type_str[type], ast_type_size[type]);
    }
    memset(node_ptr, 0, ast_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}