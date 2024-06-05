#include "irgen.h"

#include "phobos/sema.h"
#include "ptrmap.h"

static mars_module* mars_mod;
static FeModule* am;

static PtrMap entity2stackoffset;

void generate_ir_atlas_from_mars(mars_module* mod, FeModule* atmod) {
    am = atmod;
    ptrmap_init(&entity2stackoffset, 32);

    for_urange(i, 0, mod->program_tree.len) {
        if (mod->program_tree.at[i].type == AST_decl_stmt) {
            generate_ir_global_from_stmt_decl(atmod, mod->program_tree.at[i]);
        } else {
            general_error("FIXME: unhandled AST root");
        }
    }
}

FeGlobal* generate_ir_global_from_stmt_decl(FeModule* mod, AST ast) { //FIXME: add sanity
    ast_decl_stmt* decl_stmt = ast.as_decl_stmt; 

    //note: CAE problem, we update the AIR_Symbol name after AIR_Global creation

    //note: global decl_stmts are single entries on the lhs, so we can just assume that lhs[0] == ast_identifier_expr

    FeGlobal* air_g = fe_new_global(mod, NULL, /*global=*/true, /*read_only=*/decl_stmt->is_mut);
    air_g->sym->name = decl_stmt->lhs.at[0].as_identifier_expr->tok->text; 

    if (decl_stmt->rhs.type == AST_func_literal_expr) {
        FeFunction* fn = generate_ir_function(mod, decl_stmt->rhs);
        fn->sym->name = string_clone(string_concat(air_g->sym->name, str(".code")));
        fe_set_global_symref(air_g, fn->sym);
    } else { //AST_literal
        general_error("FIXME: unhandled RHS of decl_stmt, it wasnt an AST_func_literal_expr");
    }

    return air_g;
}

//FIXME: add generate_ir_local_from_stmt_decl

FeInst* generate_ir_expr_literal(FeFunction* f, FeBasicBlock* bb, AST ast) {
    ast_literal_expr* literal = ast.as_literal_expr;
    FeConst* ir = (FeConst*) fe_const(f);
    
    switch (literal->value.kind) {
    case EV_I64:
        ir->base.T = fe_type(am, FE_I64, 0);
        ir->i64 = literal->value.as_i64;
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled EV type");
        CRASH("unhandled EV type");
    }

    fe_append(bb, (FeInst*) ir);
    return (FeInst*) ir;
}

FeInst* generate_ir_expr_binop(FeFunction* f, FeBasicBlock* bb, AST ast) {
    ast_binary_op_expr* binop = ast.as_binary_op_expr;

    FeInst* lhs = generate_ir_expr_value(f, bb, binop->lhs);
    FeInst* rhs = generate_ir_expr_value(f, bb, binop->rhs);
    FeInst* ir  = fe_append(bb, fe_binop(f, FE_ADD, lhs, rhs));
    ir->T = lhs->T;

    switch (binop->op->type) {
    case TOK_ADD: ir->tag = FE_ADD; break;
    case TOK_SUB: ir->tag = FE_SUB; break;
    case TOK_MUL: ir->tag = FE_MUL; break;
    case TOK_DIV: ir->tag = FE_DIV; break;
    default:
        warning_at_node(mars_mod, ast, "unhandled binop type");
        CRASH("unhandled binop type");
        break;
    }

    return ir;
}

FeInst* generate_ir_expr_ident_load(FeFunction* f, FeBasicBlock* bb, AST ast) {
    ast_identifier_expr* ident = ast.as_identifier_expr;
    if (ident->is_discard) return fe_append(bb, fe_inst(f, FE_INVALID));

    // if a stackalloc and entityextra havent been generated, generate them

    if (ptrmap_get(&entity2stackoffset, ident->entity) == PTRMAP_NOT_FOUND) {

        if (!ident->entity->entity_type) {
            warning_at_node(mars_mod, ast, "bodge! assuming i64 type");
            ident->entity->entity_type = make_type(TYPE_I64);
        }

        // AIR* stackalloc = fe_append(bb, air_make_stackalloc(f, translate_type(am, ident->entity->entity_type)));
        FeStackObject* stackobj = fe_new_stackobject(f,  translate_type(am, ident->entity->entity_type));
        FeInst* stackoffset = fe_append(bb, fe_stackoffset(f, stackobj));

        ptrmap_put(&entity2stackoffset, ident->entity, stackoffset);
    }
    FeInst* stackoffset = ptrmap_get(&entity2stackoffset, ident->entity);

    FeInst* load = fe_load(f, stackoffset, false);
    load->T = translate_type(am, ident->entity->entity_type);

    return fe_append(bb, load);
}

FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast) {

    switch (ast.type) {
    case AST_literal_expr:    return generate_ir_expr_literal(f, bb, ast);
    case AST_binary_op_expr:  return generate_ir_expr_binop(f, bb, ast);
    case AST_identifier_expr: return generate_ir_expr_ident_load(f, bb, ast);
    case AST_paren_expr:      return generate_ir_expr_value(f, bb, ast.as_paren_expr->subexpr);
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
        break;
    }
}

FeInst* generate_ir_expr_address(FeFunction* f, FeBasicBlock* bb, AST ast) {
    switch (ast.type) {
    case AST_identifier_expr:
        if (ptrmap_get(&entity2stackoffset, ast.as_identifier_expr->entity) != PTRMAP_NOT_FOUND) {
            return ptrmap_get(&entity2stackoffset, ast.as_identifier_expr->entity);
        }
        ast_identifier_expr* ident = ast.as_identifier_expr;
        if (ptrmap_get(&entity2stackoffset, ident->entity) == PTRMAP_NOT_FOUND) {

            // generate stackalloc
            TODO("bruh");
        }
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
    }
}

void generate_ir_stmt_assign(FeFunction* f, FeBasicBlock* bb, AST ast) {
    ast_assign_stmt* assign = ast.as_assign_stmt;

    if (assign->lhs.len == 1) {
        FeInst* dest_address = generate_ir_expr_address(f, bb, assign->lhs.at[0]);

        if (assign->rhs.type == AST_call_expr) {
            // function calls need special handling
            warning_at_node(mars_mod, ast, "unhandled call in assignment");
            CRASH("unhandled call in assignment");
        }

        // this does not take into account struct assignments either, just regular ol values

        if (!(assign->rhs.type == AST_identifier_expr && assign->rhs.as_identifier_expr->is_discard)) {
            FeInst* source_val = generate_ir_expr_value(f, bb, assign->rhs);
            FeInst* store = fe_append(bb, fe_store(f, dest_address, source_val, false));
        }
    } else {
        warning_at_node(mars_mod, ast, "unhandled multi-assign");
        CRASH("unhandled multi-assign");
    }
}

void generate_ir_stmt_return(FeFunction* f, FeBasicBlock* bb, AST ast) {
    ast_return_stmt* astret = ast.as_return_stmt;
    
    // if its a plain return, we need to get the 
    // return values from the return variables
    if (astret->returns.len == 0) {
        for_urange(i, 0, astret->returns.len) {
            // god we're gonna do this i guess

            FeInst* stackoffset = ptrmap_get(&entity2stackoffset, f->returns[i]);

            FeInst* load = fe_append(bb, fe_load(f, stackoffset, false));
            load->T = ((FeStackOffset*)stackoffset)->object->t;
            FeInst* retval = fe_append(bb, fe_returnval(f, i, load));
        }
    } else {
        // this is NOT a plain return, which means that we can just get the values directly
        for_urange(i, 0, astret->returns.len) {
            FeInst* value = generate_ir_expr_value(f, bb, astret->returns.at[i]);
            FeInst* retval = fe_append(bb, fe_returnval(f, i, value));
        }
    }

    // printf("HEREEEEE\n");

    FeInst* ret = fe_return(f);
    // printf("[%p\n %p\n %p]\n", bb->len, bb->cap, bb->at);
    fe_append(bb, ret);
    
    // printf("HEREEEEE\n");

}

FeFunction* generate_ir_function(FeModule* mod, AST ast) {
    if (ast.type != AST_func_literal_expr) CRASH("ast type is not func literal");
    ast_func_literal_expr* astfunc = ast.as_func_literal_expr;

    // assume all functions are global at the moment
    FeFunction* f = air_new_function(mod, NULL, true);

    fe_set_func_params(f, astfunc->paramlen, NULL); // passing NULL means that it will allocate list but not fill anything
    for_urange(i, 0, f->params_len) {
        f->params[i]->T = translate_type(mod, astfunc->params[i]->entity_type);
    }

    fe_set_func_returns(f, astfunc->returnlen, NULL); // same here
    for_urange(i, 0, f->returns_len) {
        f->returns[i]->T = translate_type(mod, astfunc->returns[i]->entity_type);
    }

    FeBasicBlock* bb = fe_new_basic_block(f, str("begin"));

    // generate storage for param variables
    for_urange(i, 0, astfunc->paramlen) {

        if (astfunc->params[i]->entity_type->tag != TYPE_I64) {
            CRASH("only i64 params supported (for testing)");
        }
        
        /* the sequence we want to generate is:
            %1 = paramval <i>
            %2 = stackoffset <object of i>
            %3 = store <type of i> %2, %1
        */

        // this is utterly unreadable, sorry
        FeInst* paramval = fe_append(bb, fe_paramval(f, i));
        paramval->T = f->params[i]->T;

        FeStackObject* stackobj = fe_new_stackobject(f, translate_type(am, astfunc->params[i]->entity_type));
        FeInst* stackoffset = fe_append(bb, fe_stackoffset(f, stackobj));
        stackoffset->T = fe_type(mod, FE_PTR, 0);
        FeInst* store = fe_append(bb, fe_store(f, stackoffset, paramval, false));
        

        // store the entity's stackoffset
        entity* e = astfunc->params[i];
        ptrmap_put(&entity2stackoffset, e, stackoffset);
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
        
        FeStackObject* stackobj = fe_new_stackobject(f, translate_type(am, astfunc->returns[i]->entity_type));
        FeInst* stackoffset = fe_append(bb, fe_stackoffset(f, stackobj));
        stackoffset->T = fe_type(mod, FE_PTR, 0);

        FeInst* con = fe_append(bb, fe_const(f));
        con->T = f->returns[i]->T;
        ((FeConst*) con)->u64 = 0;
        
        FeInst* store = fe_append(bb, fe_store(f, stackoffset, con, false));

        // store the entity's stackalloc
        entity* e = astfunc->returns[i];
        ptrmap_put(&entity2stackoffset, e, stackoffset);
    }

    generate_ir_stmt_return(f, bb, astfunc->code_block.as_block_stmt->stmts.at[0]);

    return f;
}

FeType* translate_type(FeModule* m, type* t) {

    static PtrMap type2airtype = {0};
    if (type2airtype.keys == NULL) {
        ptrmap_init(&type2airtype, typegraph.len);
    }

    FeType* rettype = ptrmap_get(&type2airtype, t);
    if (rettype != PTRMAP_NOT_FOUND) {
        return rettype;
    }

    switch (t->tag) {
    case TYPE_NONE: return fe_type(m, FE_VOID, 0);
    case TYPE_BOOL: return fe_type(m, FE_BOOL, 0);
    case TYPE_U8:   return fe_type(m, FE_U8, 0);
    case TYPE_U16:  return fe_type(m, FE_U16, 0);
    case TYPE_U32:  return fe_type(m, FE_U32, 0);
    case TYPE_U64:  return fe_type(m, FE_U64, 0);
    case TYPE_I8:   return fe_type(m, FE_I8, 0);
    case TYPE_I16:  return fe_type(m, FE_I16, 0);
    case TYPE_I32:  return fe_type(m, FE_I32, 0);
    case TYPE_I64:  return fe_type(m, FE_I64, 0);
    case TYPE_F16:  return fe_type(m, FE_F16, 0);
    case TYPE_F32:  return fe_type(m, FE_F32, 0);
    case TYPE_F64:  return fe_type(m, FE_F64, 0);

    case TYPE_ALIAS:
        rettype = translate_type(m, t->as_reference.subtype);
        ptrmap_put(&type2airtype, t->as_reference.subtype, rettype);
        ptrmap_put(&type2airtype, t, rettype);
        return rettype;
    default:
        TODO("finish this lol");
        break;
    }

    return rettype;
}