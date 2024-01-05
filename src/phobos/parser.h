#pragma once
#define PHOBOS_PARSER_H

#include "../orbit.h"
#include "lexer.h"
#include "ast.h"

AST new_ast_node(ast_type type);