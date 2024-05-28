#include "atlas.h"
#include "passes/passes.h"


AtlasModule* atlas_new_module(string name, TargetInfo* target) {
    AtlasModule* mod = mars_alloc(sizeof(*mod));

    mod->name = name;

    mod->ir_module = air_new_module();
    mod->asm_module = asm_new_module(target);
    
    da_init(&mod->pass_queue, 8);
    atlas_sched_pass(mod, &air_pass_canon);
    
    return mod;
}

char* random_string(int len) {
    if (len < 3) {
        CRASH("random_string() needs to be called with len >= 3");
    }
    char* str = mars_alloc(len + 1);
    if (str == NULL) CRASH("Failed to allocate %d bytes for random_string", len + 1);
    str[0] = '#';
    str[len - 1] = '#';
    for_urange(i, 1, len - 1) {
        str[i] = 'A' + rand() % ('Z' - 'A'); //generates random ascii from 0x21 (!) to 0x7E (~)
    }
    str[len] = '\0';
    return str;
}