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

// allocate and zero a new AST node with an arena
AST new_ast_node(arena* restrict a, ast_type type) {
    AST node;
    void* node_ptr = arena_alloc(a, ast_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_ast_node() could not allocate AST node of type '%s' with size %d", ast_type_str[type], ast_type_size[type]);
    }
    memset(node_ptr, 0, ast_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}

void print_indent(int n) {
    FOR_RANGE_EXCL(i, 0, n) printf("    ");
}

// FOR DEBUGGING PURPOSES!! THIS IS NOT GOING TO BE MEMORY SAFE LMFAO
void dump_tree(AST node, int n) {\

    print_indent(n);

    switch (node.type) {
    case astype_invalid:
        printf("[INVALID]\n");
        break;
    case astype_identifier_expr:
        printf("identifier '%s'\n", clone_to_cstring(node.as_identifier_expr->tok->text));
        break;
    default:
        general_error("internal: dump_tree() passed an invalid node of type %d '%s'", node.type, ast_type_str[node.type]);
        break;
    }
}