#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

AST new_ast_node(parser* p, ast_type type) {
    AST node = {0};
    void* node_ptr = arena_list_alloc(&p->alloca, ast_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("new_ast_node() could not allocate AST node of type\"%s\"", ast_type_str[type]);
    }
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}