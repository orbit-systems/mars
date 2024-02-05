#pragma once
#define PHOBOS_ENTITY_H

#include "orbit.h"

#include "type.h"
#include "ast.h"

struct entity;
struct scope;

typedef u8 storage_class; enum {
    st_global_code, // for function code
    st_global_data, // for global variables
    st_local, // local variables
    st_local_volatile, // local variables that must be accessible via address
    st_local_static, // static local variables
    st_param, // function parameters
    st_type, // type (may be redundant)
};

typedef struct {
    struct scope * scope;
    mars_type * type;
    string identifier;
    AST declaration;
    storage_class storage;
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