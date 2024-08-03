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
        check_stmt(mod, trunk, global_scope);
    }
}

Type* check_stmt(mars_module* mod, AST node, entity_table* scope) {
    switch(node.type) {
        case AST_decl_stmt:
            checked_expr rhs = check_expr(mod, node.as_decl_stmt->rhs, scope);
            rhs.mutable = node.as_decl_stmt->is_mut;

            if (node.as_decl_stmt->rhs.type == AST_func_literal_expr && node.as_decl_stmt->lhs.len != 1) {
                error_at_node(mod, node, "function definition should only be assigned to one value");
            }

            node.as_decl_stmt->tg_type = rhs.type;

            if (node.as_decl_stmt->lhs.len != rhs.type->as_function.returns.len) error_at_node(mod, node, "lhs of declaration should have %d identifiers, has %d", rhs.type->as_function.returns.len, node.as_decl_stmt->lhs.len);

            foreach(AST lhs, node.as_decl_stmt->lhs) {
                if (lhs.type != AST_identifier) error_at_node(mod, lhs, "expected identifier, got %s", ast_type_str[lhs.type]);
                printf("decl: "str_fmt"\n", str_arg(lhs.as_identifier->tok->text));

                new_entity(scope, lhs.as_identifier->tok->text, lhs);
                scope->at[scope->len - 1]->is_mutable = rhs.mutable;
                scope->at[scope->len - 1]->entity_type = rhs.type->as_function.returns.at[count];
            }
            break;
        case AST_return_stmt:
            int return_count = 0;
            foreach(entity* ent, *scope) { if (ent->is_return) return_count++; }
            if (return_count != node.as_return_stmt->returns.len) error_at_node(mod, node, "function expects %d returns, return is returning %d", return_count, node.as_return_stmt->returns.len);

            foreach(AST ret, node.as_return_stmt->returns) {
                check_expr(mod, ret, scope);
            }
            return NULL;
        default:
            error_at_node(mod, node, "[check_module] unexpected token type: %s", ast_type_str[node.type]);
    }
}

checked_expr check_expr(mars_module* mod, AST node, entity_table* scope) {
    //we fill out ent with the info it needs :)
    switch(node.type) {
        case AST_func_literal_expr:
            Type* fn_type = check_func_literal(mod, node, scope);
            return (checked_expr){.expr = node, .type = fn_type};
        case AST_binary_op_expr:
            checked_expr lhs = check_expr(mod, node.as_binary_op_expr->lhs, scope);
            checked_expr rhs = check_expr(mod, node.as_binary_op_expr->rhs, scope);
            if (lhs.type->tag != rhs.type->tag) error_at_node(mod, node, "type mismatch: lhs and rhs are not the same type\nTODO: find out the type of lhs and rhs to print a more informative error");
            return (checked_expr){.expr = node, .type = lhs.type};
        case AST_identifier:
            entity* ident_ent = search_for_entity(scope, node.as_identifier->tok->text);
            if (ident_ent == NULL) error_at_node(mod, node, "undefined identifier: " str_fmt, str_arg(node.as_identifier->tok->text));
            return (checked_expr){.expr = node, .type = ident_ent->entity_type};
        default:
            error_at_node(mod, node, "[check_expr] unexpected token type: %s", ast_type_str[node.type]);
    }
}

Type* check_func_literal(mars_module* mod, AST func_literal, entity_table* scope) {
    //we need to parse the fn_type_expr and generate entities for each entity in the type, and also create a new global scope.
    //we create a new scope for this literal, and assign each new identifier to this scope. 
    
    entity_table* func_scope = new_entity_table(scope);

    Type* fn_type = make_type(TYPE_FUNCTION);

    int invalid_count = 0;
    foreach(AST_typed_field param, func_literal.as_func_literal_expr->type.as_fn_type_expr->parameters) {
        //we need to flood fill types back UP, due to how the a,b,c..:T syntax behaves on parsing
        if (is_null_AST(param.type)) invalid_count++;
        if (!is_null_AST(param.type)) {
            //we need to now flood filll types back up
            //we are abusing how foreach works here, do not repeat this
            for (int i = 0; i < invalid_count; i++) {
                func_literal.as_func_literal_expr->type.as_fn_type_expr->parameters.at[count - i - 1].type = param.type;
            }
            invalid_count = 0;
        }
    }

    foreach(AST_typed_field returns, func_literal.as_func_literal_expr->type.as_fn_type_expr->returns) {
        //we need to flood fill types back UP, due to how the a,b,c..:T syntax behaves on parsing
        if (is_null_AST(returns.type)) invalid_count++;
        if (!is_null_AST(returns.type)) {
            //we need to now flood filll types back up
            //we are abusing how foreach works here, do not repeat this
            for (int i = 0; i < invalid_count; i++) {
                func_literal.as_func_literal_expr->type.as_fn_type_expr->returns.at[count - i - 1].type = returns.type;
            }
            invalid_count = 0;
        }
    }

    //now we create the entities inside the scope, and error if there is a duplicate
    foreach(AST_typed_field param, func_literal.as_func_literal_expr->type.as_fn_type_expr->parameters) {
        //this WILL be bad for perf.
        entity* test_ent = search_for_entity(func_scope, param.field.as_identifier->tok->text);
        if (test_ent != NULL) {
            error_at_node(mod, test_ent->declaration, "identifier " str_fmt " already defined here", str_arg(param.field.as_identifier->tok->text));
        }

        new_entity(func_scope, param.field.as_identifier->tok->text, param.type);
        func_scope->at[func_scope->len - 1]->is_param = true;
        func_scope->at[func_scope->len - 1]->entity_type = ast_to_type(mod, param.type);
        type_add_param(fn_type, ast_to_type(mod, param.type));
    }

    foreach(AST_typed_field returns, func_literal.as_func_literal_expr->type.as_fn_type_expr->returns) {
        //this WILL be bad for perf.
        entity* test_ent = search_for_entity(func_scope, returns.field.as_identifier->tok->text);
        if (test_ent != NULL) {
            error_at_node(mod, test_ent->declaration, "identifier " str_fmt " already defined here", str_arg(returns.field.as_identifier->tok->text));
        }

        new_entity(func_scope, returns.field.as_identifier->tok->text, returns.type);
        func_scope->at[func_scope->len - 1]->is_return = true;
        func_scope->at[func_scope->len - 1]->entity_type = ast_to_type(mod, returns.type);
        type_add_return(fn_type, ast_to_type(mod, returns.type));
    }
    //the func literal has been parsed, now we continue down
    foreach (AST stmt, func_literal.as_func_literal_expr->code_block.as_stmt_block->stmts) {
        check_stmt(mod, stmt, func_scope);
    }
    return fn_type;
}

Type* ast_to_type(mars_module* mod, AST node) {
    switch (node.type) {
        case AST_basic_type_expr:
            switch (node.as_basic_type_expr->lit->type) {
                case TOK_TYPE_KEYWORD_I8:   { return make_type(TYPE_I8);   }
                case TOK_TYPE_KEYWORD_I16:  { return make_type(TYPE_I16);  }
                case TOK_TYPE_KEYWORD_I32:  { return make_type(TYPE_I32);  }
                case TOK_TYPE_KEYWORD_INT:
                case TOK_TYPE_KEYWORD_I64:  { return make_type(TYPE_I64);  }

                case TOK_TYPE_KEYWORD_U8:   { return make_type(TYPE_U8);   }
                case TOK_TYPE_KEYWORD_U16:  { return make_type(TYPE_U16);  }
                case TOK_TYPE_KEYWORD_U32:  { return make_type(TYPE_U32);  }
                case TOK_TYPE_KEYWORD_UINT:
                case TOK_TYPE_KEYWORD_U64:  { return make_type(TYPE_U64);  }

                case TOK_TYPE_KEYWORD_F16:  { return make_type(TYPE_F16);  }
                case TOK_TYPE_KEYWORD_F32:  { return make_type(TYPE_F32);  }
                case TOK_TYPE_KEYWORD_FLOAT:
                case TOK_TYPE_KEYWORD_F64:  { return make_type(TYPE_F64);  }
                case TOK_TYPE_KEYWORD_BOOL: { return make_type(TYPE_BOOL); }
                default: error_at_node(mod, node, "[INTERNAL COMPILER ERROR] unknown token type \"%s\" found when converting AST_basic_type_expr to integral type", token_type_str[node.as_basic_type_expr->lit->type]);
            }
        default:
            error_at_node(mod, node, "[INTERNAL COMPILER ERROR] found unknown node \"%s\" when converting AST to Type*", ast_type_str[node.type]);
    }
}

/*

check_stmt -> check_decl_stmt -> check_expr

*/