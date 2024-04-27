#include "deimos.h"
#include "passes.h"
#include "phobos/sema.h"
#include "irgen.h"

static mars_module* mars_mod;

IR_Module* ir_generate(mars_module* mod) {
	IR_Module* m = ir_new_module(mod->module_name);
    mars_mod = mod;
	
	/* do some codegen shit prolly */

    FOR_URANGE(i, 0, mod->program_tree.len) {
        if (mod->program_tree.at[i].type == AST_decl_stmt) {
            ir_generate_global_from_stmt_decl(m, mod->program_tree.at[i]);
        } else {
            general_error("FIXME: unhandled AST root");
        }
    }

	return m;
}

IR_Global* ir_generate_global_from_stmt_decl(IR_Module* mod, AST ast) { //FIXME: add sanity
    ast_decl_stmt* decl_stmt = ast.as_decl_stmt; 

    //note: CAE problem, we update the IR_Symbol name after IR_Global creation

    //note: global decl_stmts are single entries on the lhs, so we can just assume that lhs[0] == ast_identifier_expr

    
    IR_Global* ir_g = ir_new_global(mod, NULL, /*global=*/true, /*read_only=*/decl_stmt->is_mut);
    ir_g->sym->name = decl_stmt->lhs.at[0].as_identifier_expr->tok->text; 

    if (decl_stmt->rhs.type == AST_func_literal_expr) {
        IR_Function* fn = ir_generate_function(mod, decl_stmt->rhs);
        ir_set_global_symref(ir_g, fn->sym);
    } else { //AST_literal
        general_error("FIXME: unhandled RHS of decl_stmt, it wasnt an AST_func_literal_expr");
    }

    return ir_g;
}

//FIXME: add ir_generate_local_from_stmt_decl

IR* ir_generate_expr_literal(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_literal_expr* literal = ast.as_literal_expr;
    IR_Const* ir = (IR_Const*) ir_make_const(f);
    
    switch (literal->value.kind) {
    case EV_I64:
        ir->base.T = make_type(TYPE_I64);
        ir->i64 = literal->value.as_i64;
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled EV type");
        CRASH("unhandled EV type");
    }

    ir_add(bb, (IR*) ir);
    return (IR*) ir;
}

IR* ir_generate_expr_binop(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_binary_op_expr* binop = ast.as_binary_op_expr;

    IR* lhs = ir_generate_expr_value(f, bb, binop->lhs);
    IR* rhs = ir_generate_expr_value(f, bb, binop->rhs);
    IR* ir  = ir_add(bb, ir_make_binop(f, IR_ADD, lhs, rhs));
    ir->T = lhs->T;

    switch (binop->op->type) {
    case TOK_ADD: ir->tag = IR_ADD; break;
    case TOK_SUB: ir->tag = IR_SUB; break;
    case TOK_MUL: ir->tag = IR_MUL; break;
    case TOK_DIV: ir->tag = IR_DIV; break;
    default:
        warning_at_node(mars_mod, ast, "unhandled binop type");
        CRASH("unhandled binop type");
        break;
    }

    return ir;
}

IR* ir_generate_expr_ident_load(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_identifier_expr* ident = ast.as_identifier_expr;
    if (ident->is_discard) return ir_add(bb, ir_make(f, IR_INVALID));

    // if a stackalloc and entityextra havent been generated, generate them
    if (!ident->entity->stackalloc) {

        if (!ident->entity->entity_type) {
            warning_at_node(mars_mod, ast, "bodge! assuming i64 type");
            ident->entity->entity_type = make_type(TYPE_I64);
        }

        IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, ident->entity->entity_type));

        ident->entity->stackalloc = stackalloc;
    }
    IR* stackalloc = ident->entity->stackalloc;

    IR* load = ir_make_load(f, stackalloc, false);
    load->T = ident->entity->entity_type;

    return ir_add(bb, load);
}

IR* ir_generate_expr_value(IR_Function* f, IR_BasicBlock* bb, AST ast) {

    switch (ast.type) {
    case AST_literal_expr:    return ir_generate_expr_literal(f, bb, ast);
    case AST_binary_op_expr:  return ir_generate_expr_binop(f, bb, ast);
    case AST_identifier_expr: return ir_generate_expr_ident_load(f, bb, ast);
    case AST_paren_expr:      return ir_generate_expr_value(f, bb, ast.as_paren_expr->subexpr);
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
        break;
    }
}

IR* ir_generate_expr_address(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    switch (ast.type) {
    case AST_identifier_expr:
        if (ast.as_identifier_expr->entity->stackalloc != NULL) {
            return ast.as_identifier_expr->entity->stackalloc;
        }
        ast_identifier_expr* ident = ast.as_identifier_expr;
        if (!ident->entity->stackalloc) {

            // generate stackalloc
            IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, ident->entity->entity_type));
            TODO("bruh");
        }
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
    }
}

void ir_generate_stmt_assign(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_assign_stmt* assign = ast.as_assign_stmt;

    if (assign->lhs.len == 1) {
        IR* dest_address = ir_generate_expr_address(f, bb, assign->lhs.at[0]);

        if (assign->rhs.type == AST_call_expr) {
            // function calls need special handling
            warning_at_node(mars_mod, ast, "unhandled call in assignment");
            CRASH("unhandled call in assignment");
        }

        // this does not take into account struct assignments either, just regular ol values

        if (!(assign->rhs.type == AST_identifier_expr && assign->rhs.as_identifier_expr->is_discard)) {
            IR* source_val = ir_generate_expr_value(f, bb, assign->rhs);
            IR* store = ir_add(bb, ir_make_store(f, dest_address, source_val, false));
        }
    } else {
        warning_at_node(mars_mod, ast, "unhandled multi-assign");
        CRASH("unhandled multi-assign");
    }
}

void ir_generate_stmt_return(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_return_stmt* astret = ast.as_return_stmt;
    
    // if its a plain return, we need to get the 
    // return values from the return variables
    if (astret->returns.len == 0) {
        FOR_URANGE(i, 0, astret->returns.len) {
            entity* e = f->returns[i]->e;
            IR* stackalloc = e->stackalloc;

            IR* load = ir_add(bb, ir_make_load(f, stackalloc, false));
            load->T = get_target(stackalloc->T);
            IR* retval = ir_add(bb, ir_make_returnval(f, i, load));
        }
    } else {
        // this is NOT a plain return, which means that we can just get the values directly
        FOR_URANGE(i, 0, astret->returns.len) {
            IR* value = ir_generate_expr_value(f, bb, astret->returns.at[i]);
            IR* retval = ir_add(bb, ir_make_returnval(f, i, value));
            retval->T = make_type(TYPE_NONE);
        }
    }
    
    IR* ret = ir_add(bb, ir_make_return(f));
    ret->T = make_type(TYPE_NONE);
}

IR_Function* ir_generate_function(IR_Module* mod, AST ast) {
    if (ast.type != AST_func_literal_expr) CRASH("ast type is not func literal");
    ast_func_literal_expr* astfunc = ast.as_func_literal_expr;

    // assume all functions are global at the moment
    IR_Function* f = ir_new_function(mod, NULL, true);

    ir_set_func_params(f, astfunc->paramlen, NULL); // passing NULL means that it will allocate list but not fill anything
    FOR_URANGE(i, 0, f->params_len) {
        f->params[i]->e = astfunc->params[i];
    }

    ir_set_func_returns(f, astfunc->returnlen, NULL); // same here
    FOR_URANGE(i, 0, f->returns_len) {
        f->returns[i]->e = astfunc->returns[i];
    }

    IR_BasicBlock* bb = ir_new_basic_block(f, str("begin"));

    // generate storage for param variables
    FOR_URANGE(i, 0, astfunc->paramlen) {

        if (astfunc->params[i]->entity_type->tag != TYPE_I64) {
            CRASH("only i64 params supported (for testing)");
        }
        
        /* the sequence we want to generate is:
            %1 = paramval <i>
            %2 = stackalloc <type of i>
            %3 = store <type of i> %2, %1
        */


        IR* paramval = ir_add(bb, ir_make_paramval(f, i));
        paramval->T = f->params[i]->e->entity_type;
        IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, astfunc->params[i]->entity_type));
        stackalloc->T = make_type(TYPE_POINTER);
        set_target(stackalloc->T, paramval->T);
        IR* store = ir_add(bb, ir_make_store(f, stackalloc, paramval, false));
        store->T = make_type(TYPE_NONE);


        // store the entity's stackalloc
        entity* e = astfunc->params[i];
        e->stackalloc = stackalloc;
    }

    // generate storage for return variables
    FOR_URANGE(i, 0, astfunc->returnlen) {

        if (astfunc->returns[i]->entity_type->tag != TYPE_I64) {
            CRASH("only i64 returns supported (for testing)");
        }
        
        /* generate the stack alloc and set to zero
            %1 = stackalloc <type of i>
            %2 = const <type of i, 0>
            %3 = store %1, %2
        */
        
        IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, astfunc->returns[i]->entity_type));
        stackalloc->T = make_type(TYPE_POINTER);
        set_target(stackalloc->T, f->returns[i]->e->entity_type);

        IR* con = ir_add(bb, ir_make_const(f));
        con->T = f->returns[i]->e->entity_type;
        ((IR_Const*) con)->u64 = 0;
        
        IR* store = ir_add(bb, ir_make_store(f, stackalloc, con, false));
        store->T = make_type(TYPE_NONE);

        // store the entity's stackalloc
        entity* e = astfunc->returns[i];
        e->stackalloc = stackalloc;
    }

    ir_generate_stmt_return(f, bb, astfunc->code_block.as_block_stmt->stmts.at[0]);

    return f;
}