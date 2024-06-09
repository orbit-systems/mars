#pragma once
#define PHOBOS_PARSER_H

#include "orbit.h"
#include "lex.h"
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

AST parse_module_decl(parser* p);
AST parse_import_decl(parser* p);
AST parse_stmt(parser* p);
AST parse_expr(parser* p);
AST parse_decl_stmt(parser* p);
AST parse_type(parser* p);

parser make_parser(lexer* l, arena* alloca);
void parse_file(parser* p);


#define current_token(p)    ( (p)->tokens.at[(p)->current_tok])
#define peek_token(p, n)    ( (p)->tokens.at[(p)->current_tok + (n)])
#define advance_token(p)    (((p)->current_tok + 1 < (p)->tokens.len) ? ((p)->current_tok)++ : 0)
#define advance_n_tok(p, n) (((p)->current_tok + n < (p)->tokens.len) ? ((p)->current_tok)+=n : 0)

#define str_from_tokens(start, end) ((string){(start).text.raw, (end).text.raw - (start).text.raw + (end).text.len})

#define error_at_parser(p, message, ...) \
    error_at_string((p)->path, (p)->src, current_token(p).text, \
    message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token_index(p, index, message, ...) \
    error_at_string((p)->path, (p)->src, (p)->tokens.at[index].text, \
    message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token(p, token, message, ...) \
    error_at_string((p)->path, (p)->src, (token).text, \
    message __VA_OPT__(,) __VA_ARGS__)

#define error_at_AST(p, node, message, ...) \
    error_at_string((p)->path, (p)->src, str_from_tokens(*((node).base->start), *((node).base->end)), \
    message __VA_OPT__(,) __VA_ARGS__)

