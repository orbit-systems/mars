#pragma once
#define PHOBOS_PARSER_H

#include "../orbit.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct parser_s {
    arena_list alloca;
} parser;

// allocate and zero a new AST node with the parser's arena
AST new_ast_node(parser* p, ast_type type);