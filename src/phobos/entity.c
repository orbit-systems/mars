#include "entity.h"
#include "../deimos/deimos.h" //bodge

entity_table_list entity_tables;

u64 FNV_1a(string key) {
    const u64 FNV_OFFSET = 14695981039346656037ull;
    const u64 FNV_PRIME  = 1099511628211ull;

    u64 hash = FNV_OFFSET;
    for_urange(i, 0, key.len) {
        hash ^= (u64)(u8)(key.raw[i]);
        hash *= FNV_PRIME;
    }
    return hash;
}

entity_table* new_entity_table(entity_table* parent) {
    if (entity_tables.at == NULL) da_init(&entity_tables, 1);
    entity_table* et = malloc(sizeof(entity_table));
    *et = (entity_table){0};

    et->parent = parent;
    da_init(et, 1);

    et->alloca = arena_make(10*sizeof(entity));
    
    da_append(&entity_tables, et);
    return et;
}

entity* search_for_entity(entity_table* et, string ident) {
    if (et == NULL) return NULL;
    
    // for now, its linear search bc im too lazy to impl a hashmap
    for_urange(i, 0, et->len) {
        if (string_eq(et->at[i]->identifier, ident)) {
            return et->at[i];
        }
    }
    // if not found, search parent scope
    return search_for_entity(et->parent, ident);
}

entity* new_entity(entity_table* et, string ident, AST decl) {
    entity* e = arena_alloc(&et->alloca, sizeof(entity), alignof(entity));
    *e = (entity){0};
    e->identifier = ident;
    e->decl = decl;
    e->top = et;
    da_append(et, e);
    return e;
}