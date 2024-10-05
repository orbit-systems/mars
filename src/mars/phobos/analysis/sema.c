#include "sema.h"
#include "common/crash.h"

// #define LOG(...) printf(__VA_ARGS__)
#define LOG(...)

void check_module(mars_module* mod) {
    LOG("checking: "str_fmt "\n", str_arg(mod->module_name));
    foreach(mars_module* module, mod->import_list) {
        if (!module->checked) {
            check_module(module);
        }
    }

    //TODO:
    /*
        add global marks to entities
        
    */

    mod->entities = new_entity_table(NULL);

    //its Time.
    foreach(AST trunk, mod->program_tree) {
        check_stmt(mod, trunk, mod->entities);
    }
    mod->checked = true;
}

Type* check_stmt(mars_module* mod, AST node, entity_table* scope) {
    switch(node.type) {
        case AST_func_literal_expr: {
            ast_func_literal_expr* fn = node.as_func_literal_expr;
            string ident = fn->ident.base->start->text;

            if (search_for_entity(scope, ident)) 
                error_at_node(mod, fn->ident, "identifier already exists in scope");
            
            entity* fn_ent = new_entity(scope, ident, node);
            if (scope == mod.entities) fn_ent.is_global = true;

            fn_ent->entity_type = check_func_literal(mod, node, scope);
            fn_ent->is_mutable = false;
            fn->ident.as_identifier->entity = fn_ent;
            return NULL;
        } break;
        case AST_decl_stmt: {
            checked_expr rhs = check_expr(mod, node.as_decl_stmt->rhs, scope);
            // rhs.mutable = node.as_decl_stmt->is_mut;

            if (node.as_decl_stmt->rhs.type == AST_func_literal_expr && node.as_decl_stmt->lhs.len != 1) {
                error_at_node(mod, node, "function definition should only be assigned to one value");
            }
            
            Type* ast_type = rhs.type;
            
            if (!is_null_AST(node.as_decl_stmt->type)) ast_type = ast_to_type(mod, node.as_decl_stmt->type);
            
            node.as_decl_stmt->tg_type = ast_type;

            if (rhs.expr.type == AST_call_expr) {
                if (node.as_decl_stmt->lhs.len != rhs.type->as_function.returns.len) 
                    error_at_node(mod, node, "lhs of declaration should have %d identifiers, has %d", 
                        rhs.type->as_function.returns.len, node.as_decl_stmt->lhs.len);
                //we have a da of lhs, we can now type check each individually
                foreach(AST lhs, node.as_decl_stmt->lhs) {
                    if (lhs.type != AST_identifier) error_at_node(mod, lhs, "expected identifier, got %s", ast_type_str[lhs.type]);
                    LOG("decl: "str_fmt"\n", str_arg(lhs.as_identifier->tok->text));

                    if (search_for_entity(scope, lhs.as_identifier->tok->text)) error_at_node(mod, lhs, "identifier already exists in scope");

                    entity* lhs_entity = new_entity(scope, lhs.as_identifier->tok->text, lhs);
                    if (scope == mod->entities) lhs_entity->is_global = true;
                    lhs_entity->is_mutable = node.as_decl_stmt->is_mut;
                    lhs.as_identifier->entity = lhs_entity;

                    if (!is_null_AST(node.as_decl_stmt->type) && !check_type_cast_implicit(rhs.type, ast_type)) {
                        error_at_node(mod, node, "type mismatch: lhs and rhs cannot be cast to eachother\nTODO: find out the type of lhs and rhs to print a more informative error");
                    }
                    lhs_entity->entity_type = ast_type;
                }
            } else {
                LOG("rhs is of ast type: %s\n", ast_type_str[rhs.expr.type]);

                //we assume rhs len = 1
                if (node.as_decl_stmt->lhs.len != 1) error_at_node(mod, node, "expected 1 lhs, got %d", node.as_decl_stmt->lhs.len);
                AST lhs = node.as_decl_stmt->lhs.at[0];
                if (lhs.type != AST_identifier) error_at_node(mod, lhs, "expected identifier, got %s", ast_type_str[lhs.type]);
                LOG("decl: "str_fmt"\n", str_arg(lhs.as_identifier->tok->text));

                if (search_for_entity(scope, lhs.as_identifier->tok->text)) error_at_node(mod, lhs, "identifier already exists in scope");

                entity* lhs_item_entity = new_entity(scope, lhs.as_identifier->tok->text, lhs);
                if (scope == mod.entities) lhs_item_entity.is_global = true;
                lhs_item_entity->is_mutable = node.as_decl_stmt->is_mut;
                lhs.as_identifier->entity = lhs_item_entity;

                if (!is_null_AST(node.as_decl_stmt->type) && !check_type_cast_implicit(rhs.type, ast_type)) {
                    error_at_node(mod, node, "type mismatch: lhs and rhs cannot be cast to eachother\nTODO: find out the type of lhs and rhs to print a more informative error");
                }
                scope->at[scope->len - 1]->entity_type = ast_type;
            }
            return NULL;
        }
        case AST_return_stmt: {
            int return_count = 0;
            foreach(entity* ent, *scope) { if (ent->is_return) return_count++; }
            //get entities from higher scopes
            entity_table* curr_scope = scope->parent;
            while (curr_scope != NULL) {
                foreach(entity* ent, *curr_scope) { if (ent->is_return) return_count++; }    
                curr_scope = curr_scope->parent;
            }

            if (return_count != node.as_return_stmt->returns.len) error_at_node(mod, node, "function expects %d returns, return is returning %d", return_count, node.as_return_stmt->returns.len);

            foreach(AST ret, node.as_return_stmt->returns) {
                check_expr(mod, ret, scope);
            }
            return NULL;
        }
        case AST_if_stmt: {
            check_expr(mod, node.as_if_stmt->condition, scope);
            entity_table* if_stmt_scope = new_entity_table(scope);
            foreach(AST stmt, node.as_if_stmt->if_branch.as_stmt_block->stmts) check_stmt(mod, stmt, if_stmt_scope);
            return NULL;
        }
        case AST_while_stmt: {
            check_expr(mod, node.as_while_stmt->condition, scope);
            entity_table* while_stmt_scope = new_entity_table(scope);
            foreach(AST stmt, node.as_while_stmt->block.as_stmt_block->stmts) check_stmt(mod, stmt, while_stmt_scope);
            return NULL;
        }
        case AST_assign_stmt:
            checked_expr rhs = check_expr(mod, node.as_assign_stmt->rhs, scope);

            da(checked_expr) lhs_exprs;
            da_init(&lhs_exprs, node.as_assign_stmt->lhs.len);
            foreach(AST assignee, node.as_assign_stmt->lhs) {
                checked_expr checked_assignee = check_expr(mod, assignee, scope);
                if (!checked_assignee.mutable) {
                    error_at_node(mod, assignee, "cannot assign to immutable expression");
                }
                da_append(&lhs_exprs, checked_assignee);
                if (!check_assign_op(node.as_assign_stmt->op, checked_assignee.type, rhs.type)) error_at_node(mod, node, "cannot do operation "str_fmt" to lhs with rhs as param", str_arg(node.as_assign_stmt->op->text));
            }

            if (rhs.expr.type == AST_call_expr) {
                //the rhs return info will contain a function type for us to Wiggle with
                if (lhs_exprs.len != rhs.type->as_function.returns.len) error_at_node(mod, node, 
                    "type mismatch: function "str_fmt" returns %d values, lhs only has space for %d values", 
                    rhs.expr.as_call_expr->lhs.as_identifier->tok->text, rhs.type->as_function.returns.len, lhs_exprs.len);
                
                foreach(checked_expr cexpr, lhs_exprs) {
                    if (!check_type_cast_implicit(cexpr.type, rhs.type->as_function.returns.at[count])) 
                        error_at_node(mod, cexpr.expr, "type mismatch: return %d cannot be cast to lhs", count);
                    if (!check_assign_op(node.as_assign_stmt->op, rhs.type->as_function.returns.at[count], cexpr.type)) error_at_node(mod, node, "cannot do operation "str_fmt" to lhs with rhs as param", str_arg(node.as_assign_stmt->op->text));
                }

                return NULL;
            } else if (lhs_exprs.len == 1) {
                if (!check_type_cast_implicit(lhs_exprs.at[0].type, type_unalias(rhs.type))) 
                    error_at_node(mod, node, "type mismatch: rhs cannot be cast to lhs");

                return NULL;
            }  

            crash("unexpected environment in case AST_assign_stmt!\n");
        default:
            error_at_node(mod, node, "[check_module] unexpected token type: %s", ast_type_str[node.type]);
    }
    return NULL;
}

checked_expr check_expr(mars_module* mod, AST node, entity_table* scope) {
    //we fill out ent with the info it needs :)
    switch(node.type) {
        case AST_func_literal_expr:
            Type* fn_type = check_func_literal(mod, node, scope);
            node.base->T = fn_type;
            return (checked_expr){.expr = node, .type = fn_type};

        case AST_binary_op_expr:
            checked_expr lhs = check_expr(mod, node.as_binary_op_expr->lhs, scope);
            checked_expr rhs = check_expr(mod, node.as_binary_op_expr->rhs, scope);
            bool rhs_to_lhs = check_type_cast_implicit(lhs.type, rhs.type);
            bool lhs_to_rhs = check_type_cast_implicit(rhs.type, lhs.type);
            if (!rhs_to_lhs && !lhs_to_rhs) error_at_node(mod, node, "type mismatch: lhs and rhs cannot be cast to eachother\nTODO: find out the type of lhs and rhs to print a more informative error");
            Type* ret_type = operation_to_type(node.as_binary_op_expr->op);
            if (ret_type == NULL && rhs_to_lhs && !lhs_to_rhs) ret_type = lhs.type;
            if (ret_type == NULL && lhs_to_rhs && !rhs_to_lhs) ret_type = rhs.type;
            if (ret_type == NULL && rhs_to_lhs && lhs_to_rhs)  ret_type = lhs.type;
            node.base->T = ret_type;
            return (checked_expr){.expr = node, .type = ret_type};

        case AST_identifier:
            entity* ident_ent = search_for_entity(scope, node.as_identifier->tok->text);

            if (ident_ent == NULL) error_at_node(mod, node, "undefined identifier: " str_fmt, str_arg(node.as_identifier->tok->text));
            
            ident_ent->been_used = true;
            node.as_identifier->entity = ident_ent;

            node.base->T = ident_ent->entity_type;
            return (checked_expr){
                .expr = node, 
                .type = ident_ent->entity_type,
                .addressable = true,
                .mutable = ident_ent->is_mutable,
            };

        case AST_literal_expr:
            return check_literal(mod, node);

        case AST_unary_op_expr: {
            checked_expr subexpr = check_expr(mod, node.as_unary_op_expr->inside, scope);
            LOG("verifying op: "str_fmt"\n", str_arg(node.as_unary_op_expr->op->text));
            subexpr.type = type_unalias(subexpr.type);
            if (node.as_unary_op_expr->op->type == TOK_CARET) {
                if (subexpr.type->as_reference.subtype->tag == TYPE_NONE) {
                    error_at_node(mod, node.as_unary_op_expr->inside, "cannot dereference typeless ^%s pointer", subexpr.type->as_reference.mutable == true ? "mut" : "let");
                }
                node.base->T = subexpr.type->as_reference.subtype;
                return (checked_expr){
                    .expr = node, 
                    .type = subexpr.type->as_reference.subtype,
                    .mutable = subexpr.type->as_reference.mutable,
                };
            }
            error_at_node(mod, node, "unexpected op: "str_fmt, str_arg(node.as_unary_op_expr->op->text));
        }

        case AST_cast_expr: {
            Type* subtype = ast_to_type(mod, node.as_cast_expr->type);
            checked_expr subexpr = check_expr(mod, node.as_cast_expr->rhs, scope);
            if (!check_type_cast_explicit(subtype, subexpr.type)) error_at_node(mod, node.as_cast_expr->rhs, "cannot cast rhs to type");
            node.base->T = type_unalias(subtype); //TODO: lazy
            return (checked_expr){.expr = node, .type = type_unalias(subtype)};
        }

        case AST_selector_expr: {
            if (node.as_selector_expr->op->type != TOK_PERIOD) error_at_node(mod, node, "unhandled op: %s", token_type_str[node.as_selector_expr->op->type]);

            checked_expr lhs = check_expr(mod, node.as_selector_expr->lhs, scope);
            lhs.type = type_unalias(lhs.type);
            //we now need to scan to see what the rhs is, if its a selector we need to dig deeper
            if (node.as_selector_expr->rhs.type != AST_selector_expr && node.as_selector_expr->rhs.type != AST_identifier) 
                error_at_node(mod, node.as_selector_expr->rhs, "expected identifier or selector, got %s", ast_type_str[node.as_selector_expr->rhs.type]);

            Type* field_type = NULL;

            if (node.as_selector_expr->rhs.type == AST_identifier) {
                if (node.as_selector_expr->lhs.type != AST_identifier) crash("LHS of selector expr was not identifier, parser has broken!");

                foreach(TypeStructField field, lhs.type->as_aggregate.fields) {
                    if (string_eq(field.name, node.as_selector_expr->rhs.as_identifier->tok->text)) field_type = field.subtype;
                }
                if (lhs.type->tag == TYPE_SLICE) {
                    //we need to see if its len or raw, and if so we can check it correctly.
                    if (string_eq(constr("len"), node.as_selector_expr->rhs.as_identifier->tok->text)) field_type = make_type(TYPE_U64);
                    else if (string_eq(constr("raw"), node.as_selector_expr->rhs.as_identifier->tok->text)) { 
                        field_type = make_type(TYPE_POINTER);
                        field_type->as_reference.mutable = lhs.type->as_reference.mutable;
                        field_type->as_reference.subtype = lhs.type->as_reference.subtype;
                }   }

                if (!field_type) error_at_node(mod, node.as_selector_expr->rhs, "field "str_fmt" is not a field contained in this struct", str_arg(node.as_selector_expr->rhs.as_identifier->tok->text));
                //we can return raw or len here
                node.base->T = field_type;
                return (checked_expr){.expr = node, .type = field_type};
            }

            crash("check sel expr\n");
        }

        case AST_call_expr: {
            checked_expr lhs = check_expr(mod, node.as_call_expr->lhs, scope);
            if (lhs.type->tag != TYPE_FUNCTION) crash("expected function on lhs of call_expr, got type %d\n", lhs.type->tag);
            foreach(AST rhs, node.as_call_expr->params) {
                checked_expr argument = check_expr(mod, rhs, scope);
                if (!check_type_cast_implicit(lhs.type->as_function.params.at[count], argument.type)) 
                    error_at_node(mod, rhs, "cannot implicitly cast argument to parameter");
            }
            node.base->T = lhs.type;
            return (checked_expr){.expr = node, .type = lhs.type};
        }

        default:
            error_at_node(mod, node, "[check_expr] unexpected ast type: %s", ast_type_str[node.type]);
    }
    return (checked_expr){0};
}

checked_expr check_literal(mars_module* mod, AST literal) {
    exact_value* ev = alloc_exact_value(0, &mod->AST_alloca);
    switch (literal.as_literal_expr->tok->type) {
        case TOK_LITERAL_NULL:
            ev->as_pointer = 0;
            ev->kind = EV_POINTER;
            Type* pointer = make_type(TYPE_POINTER);
            pointer->as_reference.subtype = make_type(TYPE_NONE);
            pointer->as_reference.mutable = true;
            literal.base->T = pointer;
            return (checked_expr){.expr = literal, .ev = ev, .type = pointer};
        case TOK_LITERAL_INT:
            ev->as_i64 = string_strtol(literal.as_literal_expr->tok->text, 10);
            ev->kind = EV_I64;
            literal.base->T = make_type(TYPE_UNTYPED_INT); //TODO: this will be optimised, but dont be a lazy fuck kayla
            return (checked_expr){.expr = literal, .ev = ev, .type = make_type(TYPE_UNTYPED_INT)};
        case TOK_LITERAL_BOOL:
            ev->as_bool = string_cmp(constr("true"), literal.as_literal_expr->tok->text) != 0 ? 1 : 0;
            ev->kind = EV_BOOL;
            literal.base->T = make_type(TYPE_BOOL); //TODO: this will be optimised, but dont be a lazy fuck kayla
            return (checked_expr){.expr = literal, .ev = ev, .type = make_type(TYPE_BOOL)};
        default:
            error_at_node(mod, literal, "[INTERNAL COMPILER ERROR] unable to check literal " str_fmt " with type %s", str_arg(literal.as_literal_expr->tok->text), token_type_str[literal.as_literal_expr->tok->type]);
    }
    return (checked_expr){0};
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

    // (sandwich): initialize param list
    func_literal.as_func_literal_expr->paramlen = func_literal.as_func_literal_expr->type.as_fn_type_expr->parameters.len;
    func_literal.as_func_literal_expr->params = mars_alloc(sizeof(entity*) * func_literal.as_func_literal_expr->paramlen);
    
    //now we create the entities inside the scope, and error if there is a duplicate
    foreach(AST_typed_field param, func_literal.as_func_literal_expr->type.as_fn_type_expr->parameters) {
        //this WILL be bad for perf.
        entity* test_ent = search_for_entity(func_scope, param.field.as_identifier->tok->text);
        if (test_ent != NULL) {
            error_at_node(mod, test_ent->declaration, "identifier " str_fmt " already defined here", str_arg(param.field.as_identifier->tok->text));
        }

        Type* param_type = ast_to_type(mod, param.type);
        entity* param_entity = new_entity(func_scope, param.field.as_identifier->tok->text, param.field);
        param.field.as_identifier->entity = param_entity;
        param_entity->is_param = true;
        param_entity->entity_type = param_type;
        param_entity->param_idx = count;
        type_add_param(fn_type, param_type);

        // (sandwich): add the entities to the function definition
        func_literal.as_func_literal_expr->params[count] = param_entity;
    }

    // (sandwich): initialize return list
    func_literal.as_func_literal_expr->returnlen = func_literal.as_func_literal_expr->type.as_fn_type_expr->returns.len;
    func_literal.as_func_literal_expr->returns = mars_alloc(sizeof(entity*) * func_literal.as_func_literal_expr->returnlen);
    

    foreach(AST_typed_field returns, func_literal.as_func_literal_expr->type.as_fn_type_expr->returns) {
        //this WILL be bad for perf.
        entity* test_ent = search_for_entity(func_scope, returns.field.as_identifier->tok->text);
        if (test_ent != NULL) {
            error_at_node(mod, test_ent->declaration, "identifier " str_fmt " already defined here", str_arg(returns.field.as_identifier->tok->text));
        }

        Type* return_type = ast_to_type(mod, returns.type);
        entity* return_entity = new_entity(func_scope, returns.field.as_identifier->tok->text, returns.field);
        returns.field.as_identifier->entity = return_entity;
        return_entity->is_return = true;
        return_entity->entity_type = return_type;
        return_entity->return_idx = count;
        type_add_return(fn_type, return_type);

        
        // (sandwich): add the entities to the function definition
        func_literal.as_func_literal_expr->returns[count] = return_entity;
    }
    // type_canonicalize_graph();
    //the func literal has been parsed, now we continue down
    foreach (AST stmt, func_literal.as_func_literal_expr->code_block.as_stmt_block->stmts) {
        check_stmt(mod, stmt, func_scope);
    }

    //now, we look through the scope, seeing if a param or a return has been used!
    foreach(entity* ent, *func_scope) {
        if (ent->declaration.type != AST_identifier) continue;
        if (ent->is_param && !ent->been_used) warning_at_node(mod, ent->declaration, "unused param: "str_fmt, str_arg(ent->identifier));
        else if (ent->is_return && !ent->been_used) warning_at_node(mod, ent->declaration, "unused return: "str_fmt, str_arg(ent->identifier));
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
        case AST_pointer_type_expr:
            Type* pointer = make_type(TYPE_POINTER);
            pointer->as_reference.mutable = node.as_pointer_type_expr->mutable;
            if (is_null_AST(node.as_pointer_type_expr->subexpr)) pointer->as_reference.subtype = make_type(TYPE_NONE);
            else pointer->as_reference.subtype = ast_to_type(mod, node.as_pointer_type_expr->subexpr);
            return pointer;
        case AST_slice_type_expr:
            Type* slice = make_type(TYPE_SLICE);
            slice->as_reference.mutable = node.as_slice_type_expr->mutable;
            if (is_null_AST(node.as_slice_type_expr->subexpr)) slice->as_reference.subtype = make_type(TYPE_NONE);
            else slice->as_reference.subtype = ast_to_type(mod, node.as_slice_type_expr->subexpr);
            return slice;
        default:
            error_at_node(mod, node, "[INTERNAL COMPILER ERROR] found unknown node \"%s\" when converting AST to Type*", ast_type_str[node.type]);
    }
    return NULL;
}

Type* operation_to_type(token* tok) {
    switch (tok->type) {
        case TOK_CARET:         
        case TOK_ADD:           
        case TOK_SUB:           
        case TOK_MUL:           
        case TOK_DIV:           
        case TOK_MOD:           
        case TOK_MOD_MOD:       
        case TOK_TILDE:         
        case TOK_AND:           
        case TOK_OR:            
        case TOK_NOR:           
        case TOK_LSHIFT:        
        case TOK_RSHIFT:        return NULL;
        case TOK_EXCLAM:       
        case TOK_QUESTION:     
        case TOK_AND_AND:      
        case TOK_OR_OR:        
        case TOK_EQUAL_EQUAL:  
        case TOK_NOT_EQUAL:    
        case TOK_LESS_THAN:    
        case TOK_LESS_EQUAL:   
        case TOK_GREATER_THAN: 
        case TOK_GREATER_EQUAL: return make_type(TYPE_BOOL);
        default:
            general_error("[INTERNAL COMPILER ERROR] unknown token %s found when converting operation "str_fmt" to type", token_type_str[tok->type], str_arg(tok->text));
    }
    return NULL;
}

bool check_assign_op(token* op, Type* lhs, Type* rhs) {
    //all assign ops behave like operations, except we're inline
    //and so we can duplicate what we do for binary ops sort of
    lhs = type_unalias(lhs);
    rhs = type_unalias(rhs);
    if (op->type == TOK_EQUAL && check_type_cast_implicit(lhs, rhs)) return true;
    else if (lhs->tag > TYPE_META_INTEGRAL && lhs->tag == TYPE_POINTER && check_type_cast_implicit(lhs, rhs)) return true;
    else if (TYPE_NONE < lhs->tag && lhs->tag < TYPE_META_INTEGRAL && check_type_cast_implicit(lhs, rhs)) return true;
    else return false;
}

bool check_type_cast_implicit(Type* lhs, Type* rhs) {
    type_canonicalize_graph();
    lhs = type_unalias(lhs);
    rhs = type_unalias(rhs);

    LOG("lhs: %d, rhs: %d\n", lhs->tag, rhs->tag);
    if (lhs->tag == rhs->tag 
        && rhs->tag != TYPE_STRUCT
        && rhs->tag != TYPE_UNION
        && rhs->tag != TYPE_ENUM
        && rhs->tag != TYPE_UNTYPED_AGGREGATE
        && rhs->tag != TYPE_FUNCTION
        && rhs->tag != TYPE_POINTER
        && rhs->tag != TYPE_ALIAS) return true;

    if (type_equivalent(lhs, rhs, NULL)) return true;
    //we now have a situation where implicit casts are possible.
    //casting rules:
    //^mut T <-> ^mut
    //^let T <-> ^let
    //pointer types can implicitly lose and gain typing
    //enum T -> T
    //enums can be cast to their backing type
    //(u/i)N -> (u/i)M where N, M ∈ {8, 16, 32, 64}, N <= M
    //this one is a doozy, but basically says lhs can be cast to rhs, if and only if there is no loss of information
    if (lhs->tag == rhs->tag && lhs->tag == TYPE_POINTER) {
        //we check if the lhs or rhs are typed, and allow the cast if the typing is gained or dropped
        //note: mutability constraints must be obeyed, unless mut -> let
        LOG("lhs mutable: %d, rhs mutable: %d\n", lhs->as_reference.mutable, rhs->as_reference.mutable);
        if (lhs->as_reference.subtype->tag == TYPE_NONE && rhs->as_reference.subtype->tag != TYPE_NONE 
            && (lhs->as_reference.mutable == rhs->as_reference.mutable)) return true;

        if ((lhs->as_reference.subtype->tag != TYPE_NONE && rhs->as_reference.subtype->tag == TYPE_NONE) 
            && ((lhs->as_reference.mutable == rhs->as_reference.mutable) 
                || (lhs->as_reference.mutable == false && rhs->as_reference.mutable == true))) return true;

        if (lhs->as_reference.subtype == rhs->as_reference.subtype &&
            ((lhs->as_reference.mutable == rhs->as_reference.mutable)
            || (lhs->as_reference.mutable == false && rhs->as_reference.mutable == true))) return true;
    }

    //enum type backing is checked to be valid, and so we can assume T is integral already
    if (rhs->tag == TYPE_ENUM) {
        if (rhs->as_enum.backing_type->tag == lhs->tag) return true;
    }
    //now we check for implicit casts between integrals
    //we check ONLY the rhs going to lhs
    if (TYPE_I8 <= rhs->tag && rhs->tag <= TYPE_I64 
        && TYPE_I8 <= lhs->tag && lhs->tag <= TYPE_I64 
        && rhs->tag <= lhs->tag) return true;

    if (TYPE_U8 <= rhs->tag && rhs->tag <= TYPE_U64 
        && TYPE_U8 <= lhs->tag && lhs->tag <= TYPE_U64 
        && rhs->tag <= lhs->tag) return true;

    if (rhs->tag == TYPE_UNTYPED_INT && TYPE_I8 <= lhs->tag && lhs->tag <= TYPE_U64) return true;
    if ((rhs->tag == TYPE_UNTYPED_INT || (TYPE_I8 <= rhs->tag && rhs->tag <= TYPE_U64)) 
        && lhs->tag == TYPE_POINTER) return true;

    return false;
}

bool check_type_cast_explicit(Type* lhs, Type* rhs) {
    lhs = type_unalias(lhs);
    rhs = type_unalias(rhs);

    if (check_type_cast_implicit(lhs, rhs)) return true;
    //we now need to check casts that are already _not_ implicit.
    if (TYPE_UNTYPED_INT <= lhs->tag && lhs->tag <= TYPE_F64 &&
        TYPE_UNTYPED_INT <= rhs->tag && rhs->tag <= TYPE_F64) return true;

    crash("UNKNOWN CAST! FILL THIS OUT IF YOU SEE IT!\n");
    return false;
}