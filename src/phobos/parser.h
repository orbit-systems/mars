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
} parser;

// any stored entity
typedef struct entity_s {

} entity;

parser make_parser(lexer* restrict l, arena_list alloca);

AST new_ast_node(parser* restrict p, ast_type type);