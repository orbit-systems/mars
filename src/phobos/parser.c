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

void parse_module_decl(parser* restrict p) {
    if (current_token(p).type != tt_keyword_module)
        error_at_parser(p, "expected \'module\', got %s", token_type_str[current_token(p).type]);
    
    advance_token(p);
    if (current_token(p).type != tt_identifier)
        error_at_parser(p, "expected module name, got %s", token_type_str[current_token(p).type]);
    p->module_name = current_token(p).text;

    advance_token(p);
    if (current_token(p).type != tt_semicolon)
        error_at_parser(p, "expected ';', got %s", token_type_str[current_token(p).type]);
    
    advance_token(p);
    return;
}