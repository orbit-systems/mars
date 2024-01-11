#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

#define error_at_parser(p, message, ...) \
    error_at_string(p->path, p->src, current_token(p).text, message, __VA_ARGS__)

// construct a parser struct from a lexer and an arena allocator
parser make_parser(lexer* restrict l, arena alloca) {
    parser p = {0};
    p.alloca = alloca;
    p.tokens = l->buffer;
    p.path   = l->path;
    p.src    = l->src;
    p.current_token = 0;

    return p;
}

AST parse_module_decl(parser* restrict p) {

    AST n = new_ast_node(&p->alloca, astype_module_decl);

    if (current_token(p).type != tt_keyword_module)
        error_at_parser(p, "expected module declaration, got %s", token_type_str[current_token(p).type]);
    n.as_module_decl->base.start = &current_token(p);

    advance_token(p);
    if (current_token(p).type != tt_identifier)
        error_at_parser(p, "expected module name, got %s", token_type_str[current_token(p).type]);

    n.as_module_decl->name = &current_token(p);

    advance_token(p);
    if (current_token(p).type != tt_semicolon)
        error_at_parser(p, "expected ';', got %s", token_type_str[current_token(p).type]);
    
    n.as_module_decl->base.end = &current_token(p);
    advance_token(p);
    return n;
}

void parse_file(parser* restrict p) {
    
    p->module_decl = parse_module_decl(p);

    p->head = new_ast_node(&p->alloca, astype_block_stmt);

}

AST parse_expr(parser* restrict p) {
    AST n = {0};

    switch (current_token(p).type) {
    case tt_open_paren:
        n = new_ast_node(&p->alloca, astype_paren_expr);
        
        n.as_paren_expr->base.start = &current_token(p); 
        
        advance_token(p);
        n.as_paren_expr->subexpr = parse_expr(p);
        if (current_token(p).type != tt_close_paren)
            error_at_parser(p, "expected ')', got %s", token_type_str[current_token(p).type]);
        
        n.as_paren_expr->base.end = &current_token(p);
        break;
    default:
    }
    return n;
}

AST parse_stmt(parser* restrict p) {
    AST n = {0};

    switch (current_token(p).type) {
    case tt_keyword_if:
        n = new_ast_node(&p->alloca, astype_if_stmt);

    }
}

AST parse_block_stmt(parser* restrict p) {
    TODO("parse_block_stmt");
    return NULL_AST;
}