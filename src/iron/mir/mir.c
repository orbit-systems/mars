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

// use a hashmap later
FeMirSymbolIndex fe_mir_symbol_lookup(FeMirObject* obj, const char* name, u32 len) {
    for_n (i, 1, obj->symbols.len) {
        const char* sym_name = fe_mir_string(obj, obj->symbols.at[i].name);

        if (strncmp(sym_name, name, len) == 0 && sym_name[len] == '\0') {
            return (FeMirSymbolIndex){i};
        }
    }
    return FE_MIR_NULL_SYMBOL;
}

#define ADDR_AS(insts, T) ((T*)&(insts))

FeMirRelocIndex fe_mir_reloc_new(FeMirObject* obj, FeMirRelocKind kind, FeMirSymbolIndex sym) {
    u32 i = obj->relocations.len;
    vec_append(&obj->relocations, (FeMirRelocation){});
    return (FeMirRelocIndex){i};
}

FeMirRelocIndex fe_mir_reloc_new_with_addend(FeMirObject* obj, FeMirRelocKind kind, FeMirSymbolIndex sym, u32 addend) {
    u32 i = obj->relocations.len;
    vec_reserve(&obj->relocations, 2);
    obj->relocations.len += 2;

    FeMirRelocationAddend* addend_reloc = (void*)&(obj->relocations.at[i + 1]);
    addend_reloc->_kind = FE_MIR_RELOC__ADDEND;
    addend_reloc->addend = addend;

    return (FeMirRelocIndex){i};
}

FeMirObject* fe_mir_object_new(FeMirObjectKind kind) {
    FeMirObject* obj = fe_zalloc(sizeof(*obj));

    obj->kind = kind;

    // init string pool
    vec_init(&obj->string_pool, 256);
    // immediate null terminator so the null string index 
    // maps to a string of length zero.
    vec_append(&obj->string_pool, 0); 

    vec_init(&obj->sections, 8);

    // init symbol list
    vec_init(&obj->symbols, 32);
    // null entry for null symbol index
    vec_append(&obj->symbols, (FeMirSymbol){0});

    // init relocation buffer
    vec_init(&obj->relocations, 128);
    // null entry for null relocation index
    vec_append(&obj->relocations, (FeMirRelocation){0});


    fe_arena_init(&obj->arena);

    return obj;
}


FeMirSection* fe_mir_section_new(FeMirObject* obj, const char* name, u32 name_len, FeMirSectionFlags flags) {
    
    FeMirSection* section = fe_zalloc(sizeof(*section));
    section->flags = flags;
    section->name = fe_mir_intern_string(obj, name, name_len);

    vec_init(&section->elems, 128);

    return section;
}

