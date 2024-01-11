#pragma once
#define PHOBOS_PARSER_H

#include "orbit.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct parser {
    arena alloca;
    dynarr(token) tokens;
    string path;
    string src;
    size_t current_token;

    AST    module_decl;
    AST    head;
} parser;


parser make_parser(lexer* restrict l, arena alloca);

AST parse_module_decl(parser* restrict p);
void parse_file(parser* restrict p);

#define current_token(p_ptr) ((p)->tokens.raw[(p)->current_token])
#define advance_token(p_ptr) (((p)->current_token + 1 < (p)->tokens.len) ? ((p)->current_token)++ : 0)
#define advance_n_tok(p_ptr, n) (((p)->current_token + n < (p)->tokens.len) ? ((p)->current_token)+=n : 0)