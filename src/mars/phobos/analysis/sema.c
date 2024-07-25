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
                //we first need to extract type information from the rhs, just incase its a function sig
                //if its a function sig, then lhs types need to be ordered left to right, mapping
                //return values to lhs value. if these numbers arent equal, we error
                
                if (trunk.as_decl_stmt->rhs.type == AST_func_literal_expr) {
                    //we handle this like a func literal.
                    if (trunk.as_decl_stmt->lhs.len != 1) error_at_node(mod, trunk, "LHS of a function literal declaration should only have one element!");
                    check_func_literal(mod, trunk, global_scope, trunk.as_decl_stmt->lhs.at[0]);
                }

                foreach(AST lhs, trunk.as_decl_stmt->lhs) {
                    if (lhs.type != AST_identifier) error_at_node(mod, lhs, "expected identifier, got %s", ast_type_str[lhs.type]);
                    printf("decl: "str_fmt"\n", str_arg(lhs.as_identifier->tok->text));
                    check_expr(mod, lhs, global_scope);
                }
            default:
                error_at_node(mod, trunk, "[check_module] unexpected token type: %s", ast_type_str[trunk.type]);
        }
    }
}

type* check_expr(mars_module* mod, AST node, entity_table* scope) {
    //we fill out ent with the info it needs :)
    switch(node.type) {
        case AST_identifier:
            //ent->
        default:
            error_at_node(mod, node, "[check_expr] unexpected token type: %s", ast_type_str[node.type]);
    }
}

type* check_func_literal(mars_module* mod, AST decl_root, entity_table* scope, AST identifier) {
    if (decl_root.type != AST_decl_stmt) error_at_node(mod, decl_root, "[check_func_literal] INTERNAL COMPILER ERROR: got %s, expected declaration statement", ast_type_str[decl_root.type]);
    if (identifier.type != AST_identifier) error_at_node(mod, identifier, "[check_func_literal] INTERNAL COMPILER ERROR: got %s, expected identifier", ast_type_str[identifier.type]);
    
    //we need to parse the fn_type_expr and generate entities for each entity in the type, and also create a new global scope.
    //we create a new scope for this literal, and assign each new identifier to this scope. 
    
    AST node = decl_root.as_decl_stmt->rhs;

    entity* func_ent = new_entity(scope, identifier.as_identifier->tok->text, decl_root);

    foreach(AST_typed_field param, node.as_func_literal_expr->type.as_fn_type_expr->parameters) {
        printf("param: "str_fmt", type: %s\n", str_arg(param.field.as_identifier->tok->text), ast_type_str[param.type.type]);
        
    }
    entity_table* func_scope = new_entity_table(scope);

}