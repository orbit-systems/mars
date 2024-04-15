#pragma once
#define DEIMOS_H

#include "../phobos/ast.h"
#include "pass.h"

void deimos_init(AST base_node);

typedef struct {
    token* identifier;
    struct entity* entity;
    bool is_volatile;
    bool is_uninit;
} identifier_entity_pair;

da_typedef(identifier_entity_pair);