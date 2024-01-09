#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

#define error_at_parser(p, message, ...) \
    error_at_string(p->path, p->src, current_token(p).text, message, __VA_ARGS__)

// construct a parser struct from a lexer and an arena_list allocator
parser make_parser(lexer* restrict l, arena_list alloca) {
    parser p;
    p.alloca = alloca;
    p.tokens = l->buffer;
    p.path   = l->path;
    p.src    = l->src;
    p.module_name = NULL_STR;
    p.current_token = 0;

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

void parse_module_decl(parser* restrict p) {
    if (current_token(p).type != tt_keyword_module)
        error_at_parser(p, "expected \'module\', got %s", token_type_str[current_token(p).type]);
    
    advance_token(p);

    if (current_token(p).type != tt_keyword_module)
        error_at_parser(p, "expected \'module\', got %s", token_type_str[current_token(p).type]);
        
}