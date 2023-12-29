#pragma once
#define PHOBOS_AST_H

#include "../orbit.h"
#include "lexer.h"

typedef struct {
    token* start;
    token* end;
} ast_node_base;