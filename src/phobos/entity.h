#pragma once
#define PHOBOS_ENTITY_H

#include "orbit.h"
#include "phobos.h"
#include "ast.h"
#include "parser.h"
#include "type.h"

typedef struct entity {
    string identifier;
    AST decl; // If it's NULL_AST, it hasn't been declared yet.

    union {
        type* entity_type;
        mars_module* module;
    };

    exact_value* const_val;

    bool is_const   : 1;
    bool is_mutable : 1;
    bool is_type    : 1;
    bool is_module  : 1;
    bool is_extern  : 1;
    bool is_used    : 1;
    bool is_pointed_to : 1; // does its pointer ever get taken?

    bool visited    : 1; // for cyclic dependency checking
} entity;

typedef struct entity_table_list {
    struct entity_table** at;
    size_t len;
    size_t cap;
} entity_table_list;

typedef struct entity_table {
    struct entity_table* parent;
    arena alloca;

    entity** at;
    size_t len;
    size_t cap;
} entity_table;

extern entity_table_list entity_tables;

u64 FNV_1a(string key); // for implementing a hash table later

entity_table* new_entity_table(entity_table* restrict parent);

entity* search_for_entity(entity_table* restrict et, string ident);
entity* new_entity(entity_table* restrict et, string ident, AST decl);