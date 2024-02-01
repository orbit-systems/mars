#pragma once
#define PHOBOS_PARSER_H

#include "orbit.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct parser {
    arena* alloca;
    da(token) tokens;
    string path;
    string src;
    size_t current_tok;

    AST    module_decl;
    da(AST) stmts;

    size_t num_nodes;

} parser;

parser make_parser(lexer* restrict l, arena* alloca);
void parse_file(parser* restrict p);

#define new_ast_node_p(p, type) ((p)->num_nodes++, new_ast_node((p)->alloca, (type)))

AST parse_module_decl(parser* restrict p);
AST parse_stmt(parser* restrict p);
AST parse_block_stmt(parser* restrict p);
AST parse_elif(parser* restrict p);

AST parse_binary_expr(parser* restrict p, int precedence, bool no_tcl);
AST parse_non_unary_expr(parser* restrict p, AST lhs, int precedence, bool no_tcl);
AST parse_unary_expr(parser* restrict p, bool no_tcl);
AST parse_atomic_expr(parser* restrict p, bool no_tcl);
#define parse_type_expr(p) (parse_unary_expr((p), true))
#define parse_expr(p, no_cl) (parse_binary_expr(p, 0, no_cl))
