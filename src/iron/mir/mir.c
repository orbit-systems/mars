#include "common/vec.h"
#include "iron/iron.h"
#include "mir.h"

// WARNING: may be invalidated after modification
const char* fe_mir_string(FeMirObject* obj, FeMirStringIndex index) {
    return (const char*) &obj->string_pool.at[index._];
}

FeMirStringIndex fe_mir_intern_string(FeMirObject* obj, const char* name, u32 len) {
    u32 pos = obj->string_pool.len;

    vec_reserve(&obj->string_pool, len + 1);

    memcpy(&obj->string_pool.at[pos], name, len);
    obj->string_pool.len += len;
    obj->string_pool.at[obj->string_pool.len] = '\0';
    obj->string_pool.len += 1;

    return (FeMirStringIndex){pos};
}

FeMirObject* fe_mir_new_object(FeMirObjectKind kind) {
    FeMirObject* obj = fe_zalloc(sizeof(*obj));

    obj->kind = kind;

    // init string table
    vec_init(&obj->string_pool, 512);
    // zero so that null string indexes map to a string of length zero.
    vec_append(&obj->string_pool, 0); 

    // init symbol list
    vec_init(&obj->symbols, 32);
    // entry for null symbol indexes
    vec_append(&obj->symbols, (FeMirSymbol){0});


    vec_init(&obj->sections, 8);

    fe_arena_init(&obj->arena);

    return obj;
}

// use a hashmap later
u32 fe_mir_symbol_lookup(FeMirObject* obj, const char* name, u32 len) {
    for_n (i, 1, obj->symbols.len) {
        const char* sym_name = fe_mir_string(obj, obj->symbols.at[i].name);

        if (strncmp(sym_name, name, len) == 0 && sym_name[len] == '\0') {
            return i;
        }
    }
    return 0;
}

FeMirSection* fe_mir_new_section(FeMirObject* obj, const char* name, u32 name_len, FeMirSectionFlags flags) {
    
    FeMirSection* section = fe_zalloc(sizeof(*section));
    section->flags = flags;
    section->name = fe_mir_intern_string(obj, name, name_len);

    return section;
}

