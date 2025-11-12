#include "common/vec.h"
#include "iron/iron.h"
#include "mir.h"

// WARNING: may be invalidated after modification
const char* fe_mir_string(FeMirObject* obj, FeMirStringIndex index) {
    return (const char*) &obj->string_pool[index._];
}

FeMirStringIndex fe_mir_intern_string(FeMirObject* obj, const char* name, u32 len) {
    u32 pos = vec_len(obj->string_pool);

    vec_reserve(&obj->string_pool, len + 1);

    memcpy(&obj->string_pool[pos], name, len);
    vec_len(obj->string_pool) += len;
    obj->string_pool[vec_len(obj->string_pool)] = '\0';
    vec_len(obj->string_pool) += 1;

    return (FeMirStringIndex){pos};
}

// use a hashmap later
FeMirSymbolIndex fe_mir_symbol_lookup(FeMirObject* obj, const char* name, u32 len) {
    for_n (i, 1, vec_len(obj->symbols)) {
        const char* sym_name = fe_mir_string(obj, obj->symbols[i].name);

        if (strncmp(sym_name, name, len) == 0 && sym_name[len] == '\0') {
            return (FeMirSymbolIndex){i};
        }
    }
    return FE_MIR_NULL_SYMBOL;
}

#define ADDR_AS(insts, T) ((T*)&(insts))

FeMirRelocIndex fe_mir_reloc_new(FeMirObject* obj, FeMirRelocKind kind, FeMirSymbolIndex sym) {
    u32 i = vec_len(obj->relocations);
    vec_append(&obj->relocations, (FeMirRelocation){});
    return (FeMirRelocIndex){i};
}

FeMirRelocIndex fe_mir_reloc_new_with_addend(FeMirObject* obj, FeMirRelocKind kind, FeMirSymbolIndex sym, u32 addend) {
    u32 i = vec_len(obj->relocations);
    vec_reserve(&obj->relocations, 2);
    vec_len(obj->relocations) += 2;

    FeMirRelocationAddend* addend_reloc = (void*)&(obj->relocations[i + 1]);
    addend_reloc->_kind = FE_MIR_RELOC__ADDEND;
    addend_reloc->addend = addend;

    return (FeMirRelocIndex){i};
}

FeMirObject* fe_mir_object_new(FeMirObjectKind kind) {
    FeMirObject* obj = fe_zalloc(sizeof(*obj));

    obj->kind = kind;

    // init string pool
    obj->string_pool = vec_new(u8, 256);
    // immediate null terminator so the null string index 
    // maps to a string of length zero.
    vec_append(&obj->string_pool, 0); 

    obj->sections = vec_new(FeMirSection, 8);

    // init symbol list
    obj->symbols = vec_new(FeMirSymbol, 32);
    // null entry for null symbol index
    vec_append(&obj->symbols, (FeMirSymbol){0});

    // init relocation buffer
    obj->relocations = vec_new(FeMirRelocation, 128);
    // null entry for null relocation index
    vec_append(&obj->relocations, (FeMirRelocation){0});


    fe_arena_init(&obj->arena);

    return obj;
}


FeMirSection* fe_mir_section_new(FeMirObject* obj, const char* name, u32 name_len, FeMirSectionFlags flags) {
    
    FeMirSection* section = fe_zalloc(sizeof(*section));
    section->flags = flags;
    section->name = fe_mir_intern_string(obj, name, name_len);

    section->elems = vec_new(FeMirElement, 128);

    return section;
}

