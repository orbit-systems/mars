#pragma once
#define PHOBOS_ENTITY_H

#include "orbit.h"

#include "type.h"
#include "ast.h"

struct entity;
struct scope;

typedef struct {
    struct scope * scope;
    mars_type * type;
    string identifier;
    AST declaration;
} entity;

typedef struct { // these have to be pointer arrays so that they can resize without pointers to their elements anywhere else getting fucked
    struct scope ** at;
    size_t len;
    size_t cap;
} scope_list;

typedef struct {
    entity ** at;
    size_t len;
    size_t cap;
} entity_list;

typedef struct {
    entity_list entities;
    scope_list subscopes;

    struct scope * superscope;
    bool is_global;
} scope;