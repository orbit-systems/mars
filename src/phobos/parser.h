#pragma once
#define PHOBOS_PARSER_H

#include "../orbit.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct parser_s {
    arena_list alloc;
} parser;

// allocate and zero a new AST node with the parser's arena
AST new_ast_node(ast_type type);