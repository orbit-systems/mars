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
    size_t current_tok;

    AST    module_decl;
    dynarr(AST) stmts;

    size_t num_nodes;

} parser;

parser make_parser(lexer* restrict l, arena alloca);
void parse_file(parser* restrict p);

AST parse_module_decl(parser* restrict p);
AST parse_stmt(parser* restrict p);
AST parse_block_stmt(parser* restrict p);
AST parse_elif(parser* restrict p);

AST parse_expr(parser* restrict p);
AST parse_binary_expr(parser* restrict p, int precedence);
AST parse_non_unary_expr(parser* restrict p, AST lhs, int precedence);
AST parse_unary_expr(parser* restrict p, bool type_expr);
AST parse_atomic_expr(parser* restrict p, bool type_expr);
#define parse_type_expr(p) (parse_unary_expr((p), true))