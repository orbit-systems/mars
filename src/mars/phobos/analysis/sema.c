#include "sema.h"

void check_module(mars_module* mod) {
    printf("checking: "str_fmt "\n", str_arg(mod->module_name));
    foreach(mars_module* module, mod->import_list) {
        if (!module->checked) {
            check_module(module);
        }
    }

    entity_table* global_scope = new_entity_table(NULL);

    //its Time.
    foreach(AST trunk, mod->program_tree) {
        switch(trunk.type) {
            case AST_decl_stmt:
                printf("\n");
                foreach(AST lhs, trunk.as_decl_stmt->lhs) {
                    printf("decl: "str_fmt"\n", str_arg(lhs.as_identifier->tok->text));
                    entity* lhs_entity = new_entity(global_scope, lhs.as_identifier->tok->text, trunk);
                    check_expr(mod, trunk, lhs_entity);
                }
            default:
                error_at_node(mod, trunk, "[check_module] unexpected token type: %s", ast_type_str[trunk.type]);
        }
    }
}

type* check_expr(mars_module* mod, AST node, entity* ent) {
    //we fill out ent with the info it needs :)
    switch(node.type) {
        case AST_func_literal_expr:
            return check_func_literal(mod, node, ent);

        default:
            error_at_node(mod, node, "[check_expr] unexpected token type: %s", ast_type_str[node.type]);
    }
}

type* check_func_literal(mars_module* mod, AST node, entity* ent) {
    if (node.type != AST_func_literal_expr) error_at_node(mod, node, "[check_func_literal] INTERNAL COMPILER ERROR:\n got %s, expected function literal", ast_type_str[node.type]);
    //we need to parse the fn_type_expr and generate entities for each entity in the type, and also create a new global scope.
    //we create a new scope for this literal, and assign each new identifier to this scope. 
    foreach(AST_typed_field param, node.as_func_literal_expr->type.as_fn_type_expr->parameters) {

    }
}