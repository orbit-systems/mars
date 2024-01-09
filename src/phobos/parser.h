#pragma once
#define PHOBOS_PARSER_H

#include "../orbit.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct parser_s {
    arena_list alloca;
    dynarr(token) tokens;
    string path;
    string src;
    string module_name;

    size_t current_token;
} parser;

// any stored entity
typedef struct entity_s {

} entity;

parser make_parser(lexer* restrict l, arena_list alloca);

AST new_ast_node(parser* restrict p, ast_type type);

void parse_module_decl(parser* restrict p);


#define current_token(p_ptr) ((p)->tokens.raw[(p)->current_token])
#define advance_token(p_ptr) (((p)->current_token + 1 < (p)->tokens.len) ? ((p)->current_token)++ : 0)
#define advance_n_tok(p_ptr, n) (((p)->current_token + n < (p)->tokens.len) ? ((p)->current_token)+=n : 0)