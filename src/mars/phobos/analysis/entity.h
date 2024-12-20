#pragma once
#define PHOBOS_ENTITY_H

#include "common/orbit.h"
#include "mars/phobos/phobos.h"
#include "mars/phobos/ast.h"
#include "mars/phobos/parse/parse.h"
#include "type.h"

typedef struct entity entity;
typedef struct entity_table_list entity_table_list;
typedef struct entity_table entity_table;

typedef struct entity {
    string identifier;

    AST declaration;

    union {
        Type* entity_type;
        mars_module* module;
    };

    exact_value* const_val;
    entity_table* top; // scope in which it is declared
    union {
        u16 param_idx;
        u16 return_idx;
    };

    bool is_param : 1;
    bool is_return : 1;
    bool been_used : 1;

    bool is_const : 1;
    bool is_mutable : 1;
    bool is_type : 1;
    bool is_module : 1;
    bool is_extern : 1;
    bool is_used : 1;
    bool is_pointed_to : 1; // does its pointer ever get taken?
    bool is_global : 1;

    bool checked : 1;
    bool visited : 1; // for cyclic dependency checking
} entity;

typedef struct entity_table_list {
    entity_table** at;
    size_t len;
    size_t cap;
} entity_table_list;

typedef struct entity_table {
    AST origin;
    entity_table* parent;
    Arena alloca;

    entity** at;
    size_t len;
    size_t cap;
} entity_table;

extern entity_table_list entity_tables;

u64 FNV_1a(string key); // for implementing a hash table later

entity_table* new_entity_table(entity_table* parent);

entity* search_for_entity(entity_table* et, string ident);
entity* new_entity(entity_table* et, string ident, AST decl);