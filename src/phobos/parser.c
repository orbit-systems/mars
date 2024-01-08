#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

// construct a parser struct from a lexer and an arena_list allocator
parser make_parser(lexer* restrict l, arena_list alloca) {
    parser p;
    p.alloca = alloca;
    p.tokens = l->buffer;
    p.path   = l->path;
    p.src    = l->src;
    return p;
}

// allocate and zero a new AST node with the parser's arena
AST new_ast_node(parser* restrict p, ast_type type) {
    AST node;
    void* node_ptr = arena_list_alloc(&p->alloca, ast_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("new_ast_node() could not allocate AST node of type\"%s\"", ast_type_str[type]);
    }
    memset(node_ptr, 0, ast_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}