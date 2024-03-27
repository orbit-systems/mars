#include "orbit.h"
#include "mars.h"

#include "phobos.h"
#include "checker.h"

void check_program(mars_module* restrict mod) {
    FOR_URANGE(i, 0, mod->import_list.len) {
        if (!mod->import_list.at[i]->checked) {
            check_program(mod->import_list.at[i]);
        }
    }
    // all imported modules have been checked
    check_module(mod);
}

// assumes imports have all been checked
void check_module(mars_module* restrict mod) {
    // printf("checking module "str_fmt"\n", str_arg(mod->module_name));

    // TODO use entities from imported modules
    entity_table* globals = new_entity_table(NULL);


    mod->checked = true;
}

void collect_globals(mars_module* restrict mod, entity_table* restrict et) {
    
}