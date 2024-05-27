#include "atlas.h"
#include "passes/passes.h"

#include "targets/aphelion/aphelion.h"

da(Pass) atlas_passes;

void add_pass(char* name, void* callback, pass_type type) {
    if (atlas_passes.at == NULL) da_init(&atlas_passes, 1);
    da_append(&atlas_passes, ((Pass){.name = name, .callback = callback, .type = type}));
}

void register_passes() {
    add_pass("general cleanup & canonicalization", air_pass_canon, PASS_AIR_TO_IR);

    bool opt = true;
    if (opt) {
        add_pass("trivial redundant memory elimination", air_pass_trme, PASS_AIR_TO_IR);
        add_pass("trivial dead code elimination", air_pass_tdce, PASS_AIR_TO_IR);
        add_pass("mov propogation", air_pass_movprop, PASS_AIR_TO_IR);
        add_pass("remove eliminated instructions", air_pass_elim, PASS_AIR_TO_IR);
    }
}

void run_passes(AIR_Module* current_program) {
    air_print_module(current_program);
    for_urange(i, 0, atlas_passes.len) {
        printf("Running pass: %s\n", atlas_passes.at[i].name);
        if (atlas_passes.at[i].type == PASS_AIR_TO_IR) {
            current_program = atlas_passes.at[i].air_callback(current_program);
        } else {
            general_error("Unexpected pass type!");
        }
        air_print_module(current_program);
    }
    //HACK
    aphelion_translate_module(current_program);
    printf("TODO: FIX THIS FUCKING HACK\n");
}