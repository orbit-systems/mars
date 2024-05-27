#include "atlas.h"
#include "phobos/sema.h"
#include "irgen.h"

static mars_module* mars_mod;

AtlasModule* generate_atlas_from_mars(mars_module* mod) {
    AtlasModule* m = atlas_new_module(mod->module_name);
    mars_mod = mod;
    
    /* do some codegen shit prolly */

    for_urange(i, 0, mod->program_tree.len) {
        if (mod->program_tree.at[i].type == AST_decl_stmt) {
            air_generate_global_from_stmt_decl(&m->ir_module, mod->program_tree.at[i]);
        } else {
            general_error("FIXME: unhandled AST root");
        }
    }

    return m;
}

AIR_Global* air_generate_global_from_stmt_decl(AIR_Module* mod, AST ast) { //FIXME: add sanity
    ast_decl_stmt* decl_stmt = ast.as_decl_stmt; 

    //note: CAE problem, we update the AIR_Symbol name after AIR_Global creation

    //note: global decl_stmts are single entries on the lhs, so we can just assume that lhs[0] == ast_identifier_expr

    AIR_Global* air_g = air_new_global(mod, NULL, /*global=*/true, /*read_only=*/decl_stmt->is_mut);
    air_g->sym->name = decl_stmt->lhs.at[0].as_identifier_expr->tok->text; 

    if (decl_stmt->rhs.type == AST_func_literal_expr) {
        AIR_Function* fn = air_generate_function(mod, decl_stmt->rhs);
        fn->sym->name = string_clone(string_concat(air_g->sym->name, str(".code")));
        air_set_global_symref(air_g, fn->sym);
    } else { //AST_literal
        general_error("FIXME: unhandled RHS of decl_stmt, it wasnt an AST_func_literal_expr");
    }

    return air_g;
}

//FIXME: add air_generate_local_from_stmt_decl

AIR* air_generate_expr_literal(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {
    ast_literal_expr* literal = ast.as_literal_expr;
    AIR_Const* ir = (AIR_Const*) air_make_const(f);
    
    switch (literal->value.kind) {
    case EV_I64:
        ir->base.T = make_type(TYPE_I64);
        ir->i64 = literal->value.as_i64;
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled EV type");
        CRASH("unhandled EV type");
    }

    air_add(bb, (AIR*) ir);
    return (AIR*) ir;
}

AIR* air_generate_expr_binop(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {
    ast_binary_op_expr* binop = ast.as_binary_op_expr;

    AIR* lhs = air_generate_expr_value(f, bb, binop->lhs);
    AIR* rhs = air_generate_expr_value(f, bb, binop->rhs);
    AIR* ir  = air_add(bb, air_make_binop(f, AIR_ADD, lhs, rhs));
    ir->T = lhs->T;

    switch (binop->op->type) {
    case TOK_ADD: ir->tag = AIR_ADD; break;
    case TOK_SUB: ir->tag = AIR_SUB; break;
    case TOK_MUL: ir->tag = AIR_MUL; break;
    case TOK_DIV: ir->tag = AIR_DIV; break;
    default:
        warning_at_node(mars_mod, ast, "unhandled binop type");
        CRASH("unhandled binop type");
        break;
    }

    return ir;
}

AIR* air_generate_expr_ident_load(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {
    ast_identifier_expr* ident = ast.as_identifier_expr;
    if (ident->is_discard) return air_add(bb, air_make(f, AIR_INVALID));

    // if a stackalloc and entityextra havent been generated, generate them
    if (!ident->entity->stackalloc) {

        if (!ident->entity->entity_type) {
            warning_at_node(mars_mod, ast, "bodge! assuming i64 type");
            ident->entity->entity_type = make_type(TYPE_I64);
        }

        AIR* stackalloc = air_add(bb, air_make_stackalloc(f, ident->entity->entity_type));

        ident->entity->stackalloc = stackalloc;
    }
    AIR* stackalloc = ident->entity->stackalloc;

    AIR* load = air_make_load(f, stackalloc, false);
    load->T = ident->entity->entity_type;

    return air_add(bb, load);
}

AIR* air_generate_expr_value(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {

    switch (ast.type) {
    case AST_literal_expr:    return air_generate_expr_literal(f, bb, ast);
    case AST_binary_op_expr:  return air_generate_expr_binop(f, bb, ast);
    case AST_identifier_expr: return air_generate_expr_ident_load(f, bb, ast);
    case AST_paren_expr:      return air_generate_expr_value(f, bb, ast.as_paren_expr->subexpr);
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
        break;
    }
}

AIR* air_generate_expr_address(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {
    switch (ast.type) {
    case AST_identifier_expr:
        if (ast.as_identifier_expr->entity->stackalloc != NULL) {
            return ast.as_identifier_expr->entity->stackalloc;
        }
        ast_identifier_expr* ident = ast.as_identifier_expr;
        if (!ident->entity->stackalloc) {

            // generate stackalloc
            AIR* stackalloc = air_add(bb, air_make_stackalloc(f, ident->entity->entity_type));
            TODO("bruh");
        }
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
    }
}

void air_generate_stmt_assign(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {
    ast_assign_stmt* assign = ast.as_assign_stmt;

    if (assign->lhs.len == 1) {
        AIR* dest_address = air_generate_expr_address(f, bb, assign->lhs.at[0]);

        if (assign->rhs.type == AST_call_expr) {
            // function calls need special handling
            warning_at_node(mars_mod, ast, "unhandled call in assignment");
            CRASH("unhandled call in assignment");
        }

        // this does not take into account struct assignments either, just regular ol values

        if (!(assign->rhs.type == AST_identifier_expr && assign->rhs.as_identifier_expr->is_discard)) {
            AIR* source_val = air_generate_expr_value(f, bb, assign->rhs);
            AIR* store = air_add(bb, air_make_store(f, dest_address, source_val, false));
        }
    } else {
        warning_at_node(mars_mod, ast, "unhandled multi-assign");
        CRASH("unhandled multi-assign");
    }
}

void air_generate_stmt_return(AIR_Function* f, AIR_BasicBlock* bb, AST ast) {
    ast_return_stmt* astret = ast.as_return_stmt;
    
    // if its a plain return, we need to get the 
    // return values from the return variables
    if (astret->returns.len == 0) {
        for_urange(i, 0, astret->returns.len) {
            entity* e = f->returns[i]->e;
            AIR* stackalloc = e->stackalloc;

            AIR* load = air_add(bb, air_make_load(f, stackalloc, false));
            load->T = get_target(stackalloc->T);
            AIR* retval = air_add(bb, air_make_returnval(f, i, load));
        }
    } else {
        // this is NOT a plain return, which means that we can just get the values directly
        for_urange(i, 0, astret->returns.len) {
            AIR* value = air_generate_expr_value(f, bb, astret->returns.at[i]);
            AIR* retval = air_add(bb, air_make_returnval(f, i, value));
            retval->T = make_type(TYPE_NONE);
        }
    }
    
    AIR* ret = air_add(bb, air_make_return(f));
    ret->T = make_type(TYPE_NONE);
}

AIR_Function* air_generate_function(AIR_Module* mod, AST ast) {
    if (ast.type != AST_func_literal_expr) CRASH("ast type is not func literal");
    ast_func_literal_expr* astfunc = ast.as_func_literal_expr;

    // assume all functions are global at the moment
    AIR_Function* f = air_new_function(mod, NULL, true);

    air_set_func_params(f, astfunc->paramlen, NULL); // passing NULL means that it will allocate list but not fill anything
    for_urange(i, 0, f->params_len) {
        f->params[i]->e = astfunc->params[i];
        f->params[i]->T = f->params[i]->e->entity_type;
    }

    air_set_func_returns(f, astfunc->returnlen, NULL); // same here
    for_urange(i, 0, f->returns_len) {
        f->returns[i]->e = astfunc->returns[i];
        f->returns[i]->T = f->returns[i]->e->entity_type;
    }

    AIR_BasicBlock* bb = air_new_basic_block(f, str("begin"));

    // generate storage for param variables
    for_urange(i, 0, astfunc->paramlen) {

        if (astfunc->params[i]->entity_type->tag != TYPE_I64) {
            CRASH("only i64 params supported (for testing)");
        }
        
        /* the sequence we want to generate is:
            %1 = paramval <i>
            %2 = stackalloc <type of i>
            %3 = store <type of i> %2, %1
        */


        AIR* paramval = air_add(bb, air_make_paramval(f, i));
        paramval->T = f->params[i]->e->entity_type;
        AIR* stackalloc = air_add(bb, air_make_stackalloc(f, astfunc->params[i]->entity_type));
        stackalloc->T = make_type(TYPE_POINTER);
        set_target(stackalloc->T, paramval->T);
        AIR* store = air_add(bb, air_make_store(f, stackalloc, paramval, false));
        store->T = make_type(TYPE_NONE);


        // store the entity's stackalloc
        entity* e = astfunc->params[i];
        e->stackalloc = stackalloc;
    }

    // generate storage for return variables
    for_urange(i, 0, astfunc->returnlen) {

        if (astfunc->returns[i]->entity_type->tag != TYPE_I64) {
            CRASH("only i64 returns supported (for testing)");
        }
        
        /* generate the stack alloc and set to zero
            %1 = stackalloc <type of i>
            %2 = const <type of i, 0>
            %3 = store %1, %2
        */
        
        AIR* stackalloc = air_add(bb, air_make_stackalloc(f, astfunc->returns[i]->entity_type));
        stackalloc->T = make_type(TYPE_POINTER);
        set_target(stackalloc->T, f->returns[i]->e->entity_type);

        AIR* con = air_add(bb, air_make_const(f));
        con->T = f->returns[i]->e->entity_type;
        ((AIR_Const*) con)->u64 = 0;
        
        AIR* store = air_add(bb, air_make_store(f, stackalloc, con, false));
        store->T = make_type(TYPE_NONE);

        // store the entity's stackalloc
        entity* e = astfunc->returns[i];
        e->stackalloc = stackalloc;
    }

    air_generate_stmt_return(f, bb, astfunc->code_block.as_block_stmt->stmts.at[0]);

    return f;
}