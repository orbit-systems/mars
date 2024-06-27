#include "sema.h"

void check_module(mars_module* mod) {
    printf("checking: "str_fmt "\n", str_arg(mod->module_name));
    foreach(mars_module* module, mod->import_list) {
        if (!module->checked) {
            check_module(module);
        }
    }

    //its Time.
    foreach(AST trunk, mod->program_tree) {
        switch(trunk.type) {
            default:
                error_at_node(mod, trunk, "unexpected token type: %s", ast_type_str[trunk.type]);
        }
    }
}