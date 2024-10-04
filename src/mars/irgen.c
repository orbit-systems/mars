#include "irgen.h"

static IrBuilder new_builder(mars_module* mars, FeModule* mod) {
    IrBuilder builder = {0};
    builder.mars = mars;
    builder.mod = mod;
    ptrmap_init(&builder.entity2stackobj, 128);
    return builder;
}

// makes a basic block name
static string next_block_name() {
    static i64 uid = 0;
    string str = strprintf("block%lld", uid++);
    return str;
}

FeModule* irgen_module(mars_module* mars) {
    FeModule* mod = fe_new_module(mars->module_name);

    IrBuilder builder = new_builder(mars, mod);

    foreach(AST decl, mars->program_tree) {
        switch (decl.type) {
        case AST_decl_stmt:
            irgen_global_decl(&builder, decl.as_decl_stmt);
            break;
        case AST_func_literal_expr:
            irgen_global_fn_decl(&builder, decl.as_func_literal_expr);
            break;
        }
    }

    return mod;
}

void irgen_global_fn_decl(IrBuilder* builder, ast_func_literal_expr* func) {
    string mars_identifier = func->ident.as_identifier->tok->text;
    string global_sym_str = irgen_mangle_identifer(builder, mars_identifier);
    
    FeSymbol* global_sym = fe_new_symbol(builder->mod, global_sym_str, FE_BIND_EXPORT);
    irgen_function(builder, func, global_sym);
}

string irgen_anonymous_fn_identifier(IrBuilder* builder) {
    static u64 counter = 0;
    string str = string_alloc(strlen("_anon_") + 16);
    memcpy(str.raw, "_anon_", strlen("_anon_"));
    
    for_range(i, 1, 17) {
        u8 digit = (counter >> (64 - i*4)) && 0x0Full;
        if (digit < 10) {
            str.raw[i + strlen("_anon_") - 1] = digit + '0';
        } else {
            str.raw[i + strlen("_anon_") - 1] = digit + 'A';
        }
    }

    counter++;
    return str;
}

// makes turns a mars identifier into a mangled/transformed symbol name
string irgen_mangle_identifer(IrBuilder* builder, string ident) {
    string module_name = builder->mars->module_name;
    u64 len = ident.len + module_name.len + 1; // + 1 for the '.'
    string str = string_alloc(len);
    memcpy(str.raw, module_name.raw, module_name.len);
    str.raw[module_name.len] = '.';
    memcpy(str.raw + module_name.len + 1, ident.raw, ident.len);
    return str;
}

// generates a global declaration.
void irgen_global_decl(IrBuilder* builder, ast_decl_stmt* global_decl) {
    // assume lhs.len == 1, since multiple lhs requires function calls
    // which are illegal at the global scope
    // cause globals must be initialized to comptime constant vals
    string mars_identifier = global_decl->lhs.at[0].as_identifier->tok->text;
    string global_sym_str = irgen_mangle_identifer(builder, mars_identifier);

    FeSymbol* global_sym = fe_new_symbol(builder->mod, global_sym_str, FE_BIND_EXPORT);
    FeData* global_data = fe_new_data(builder->mod, global_sym, !global_decl->is_mut);
    
    switch (global_decl->rhs.type) {
    case AST_func_literal_expr:
        FeSymbol* fn_sym = fe_new_symbol(builder->mod, irgen_anonymous_fn_identifier(builder), FE_BIND_LOCAL);
        FeFunction* fn = irgen_function(builder, global_decl->rhs.as_func_literal_expr, fn_sym);
        fe_set_data_symref(global_data, fn_sym);
        break;
    default:
        crash("unhandled global rhs '%s'", ast_type_str[global_decl->rhs.type]);
    }
}

// this will overwrite the current function and basic block
FeFunction* irgen_function(IrBuilder* builder, ast_func_literal_expr* fn_literal, FeSymbol* sym) {
    FeFunction* fn = fe_new_function(builder->mod, sym, FE_CALLCONV_MARS);
    builder->fn = fn;

    fe_init_func_params(fn, fn_literal->paramlen);
    fe_init_func_returns(fn, fn_literal->returnlen);

    FeBasicBlock* entry_bb = fe_new_basic_block(fn, next_block_name());
    builder->bb = entry_bb;

    FeIr** paramvals = mars_alloc(sizeof(FeIr*) * fn_literal->paramlen);

    // initialize FeFunction params and add the paramval instructions
    for_range (i, 0, fn_literal->paramlen) {
        entity* param_entity = fn_literal->params[i];
        FeType param_fe_type = irgen_mars_to_iron_type(builder, param_entity->entity_type);
    
        // add param to function definition
        fe_add_func_param(fn, param_fe_type);
        // add the paramval inst
        paramvals[i] = fe_append_ir(entry_bb, fe_ir_paramval(fn, i));
    }

    // store all the params in stack objects.
    for_range (i, 0, fn_literal->paramlen) {
        entity* param_entity = fn_literal->params[i];
        FeFunctionItem* param = fn->params.at[i];

        // create the stack object for the param
        FeStackObject* param_stack_object = fe_new_stackobject(fn, param->type);
        // add it to the builder's ptrmap
        ptrmap_put(&builder->entity2stackobj, param_entity, param_stack_object);
        // store in stack object
        fe_append_ir(entry_bb, fe_ir_stack_store(fn, param_stack_object, paramvals[i]));
    }

    mars_free(paramvals);

    // for kayla: the reason these are two different loops is because
    // irons needs the paramval instructions at the beginning of the
    // basic block, so it needs to emit all the paramvals and then emit the stack_stores.

    // initialize FeFunction returns and create stack objects
    builder->fn_returns = mars_alloc(sizeof(FeStackObject*) * fn_literal->returnlen);
    builder->fn_returns_len = fn_literal->returnlen;
    for_range (i, 0, fn_literal->returnlen) {
        entity* return_entity = fn_literal->returns[i];
        FeType return_fe_type = irgen_mars_to_iron_type(builder, return_entity->entity_type);
    
        // add param to function definition
        fe_add_func_return(fn, return_fe_type);
        // create the stack object for the return
        FeStackObject* return_stack_object = fe_new_stackobject(fn, return_fe_type);
        builder->fn_returns[i] = return_stack_object;
        
        // initialize to zero.
        FeIrConst* initial_zero = (FeIrConst*) fe_append_ir(entry_bb, fe_ir_const(fn, return_fe_type));
        initial_zero->i64 = 0; // TODO this is probably safe since all the i8, i16, etc. occupy the same space?
        fe_append_ir(entry_bb, fe_ir_stack_store(fn, return_stack_object, (FeIr*)initial_zero));


    }

    irgen_block(builder, fn_literal->code_block.as_stmt_block);
    
    return fn;
}

static bool is_mars_type_unsigned(Type* t) {
    switch (t->tag) {
    case TYPE_U8:
    case TYPE_U16:
    case TYPE_U32:
    case TYPE_U64:
    case TYPE_POINTER:
        return true;
    }
    return false;
}

// get the value of an expression.
// this is different from generate_place_expr, which returns a pointer to load/store from.
FeIr* irgen_value_expr(IrBuilder* builder, AST expr) {
    switch (expr.type) {
    case AST_binary_op_expr:
        ast_binary_op_expr* binop = expr.as_binary_op_expr;

        // generate lhs and rhs
        FeIr* lhs = irgen_value_expr(builder, binop->lhs);
        FeIr* rhs = irgen_value_expr(builder, binop->rhs);

        // select binop kind
        u8 kind = 0;
        switch (binop->op->type) {
        case TOK_ADD: kind = FE_IR_ADD; break;
        case TOK_LESS_THAN: kind = FE_IR_ILT; break;
        default: 
            crash("unhandled binop '%s'", token_type_str[binop->op->type]);
        }
        // combine
        return fe_append_ir(builder->bb, fe_ir_binop(builder->fn, kind, lhs, rhs));

    case AST_identifier:
        ast_identifier* ident = expr.as_identifier;

        general_warning("TODO: rn it makes the assumption that all variables/identifiers are stack allocated.");
        general_warning("      sema will need to put info about whether something is global/static or not in the entity.");
        // get stack object for variable
        FeStackObject* obj = ptrmap_get(&builder->entity2stackobj, ident->entity);
        if (obj == PTRMAP_NOT_FOUND) crash("ptrmap could not find stack object of entity");
        // load
        return fe_append_ir(builder->bb, fe_ir_stack_load(builder->fn, obj));
    default:
        crash("unhandled expr '%s'", ast_type_str[expr.type]);
        return NULL;
    }
}

void irgen_stmt(IrBuilder* builder, AST stmt) {
    switch (stmt.type) {
    case AST_return_stmt:
        ast_return_stmt* ret = stmt.as_return_stmt;

        // if there are immediate return values,
        // we get them and store them into their stack objects
        foreach(AST expr, ret->returns) {
            FeIr* value = irgen_value_expr(builder, expr);
            fe_append_ir(builder->bb, fe_ir_stack_store(builder->fn, builder->fn_returns[count], value));
        }

        // regardless of if there are immediate returns,
        // we get the values from the stack objects 
        // and put them in returnvals

        FeIr** return_values = mars_alloc(sizeof(FeIr*) * builder->fn_returns_len);
        for_urange (i, 0, builder->fn_returns_len) {
            return_values[i] = fe_append_ir(builder->bb, fe_ir_stack_load(builder->fn, builder->fn_returns[i]));
        }
        for_urange (i, 0, builder->fn_returns_len) {
            fe_append_ir(builder->bb, fe_ir_returnval(builder->fn, i, return_values[i]));
        }
        mars_free(return_values);
        // return lmao
        fe_append_ir(builder->bb, fe_ir_return(builder->fn));
        break;
    case AST_if_stmt:
        ast_if_stmt* ifstmt = stmt.as_if_stmt;
        
        // TODO("on hold until i rework branches in iron");
        
        FeIr* cond = irgen_value_expr(builder, ifstmt->condition);

        FeBasicBlock* if_true = fe_new_basic_block(builder->fn, next_block_name());
        FeBasicBlock* if_false = fe_new_basic_block(builder->fn, next_block_name());

        FeIrBranch* branch = (FeIrBranch*) fe_append_ir(builder->bb, fe_ir_branch(builder->fn, cond, if_true, if_false));

        FeBasicBlock* previous_backlink = builder->backlink; // save backlink
        builder->backlink = if_false;

        builder->bb = if_true;        
        irgen_stmt(builder, ifstmt->if_branch);

        builder->backlink = previous_backlink; // reset backlink
        builder->bb = if_false;

        break;
    case AST_stmt_block:
        irgen_block(builder, stmt.as_stmt_block);
        break;
    default:
        crash("unhandled stmt '%s'", ast_type_str[stmt.type]);
    }
}

// uses current basic block
void irgen_block(IrBuilder* builder, ast_stmt_block* block) {
    foreach(AST stmt, block->stmts) {
        irgen_stmt(builder, stmt);
    }

}

FeType irgen_mars_to_iron_type(IrBuilder* builder, Type* t) {
    switch (t->tag) {
    case TYPE_I64:
    case TYPE_U64: 
        return FE_TYPE_I64;
    case TYPE_I32:
    case TYPE_U32: 
        return FE_TYPE_I32;
    case TYPE_I16:
    case TYPE_U16: 
        return FE_TYPE_I16;
    case TYPE_I8:
    case TYPE_U8: 
        return FE_TYPE_I8;
    case TYPE_POINTER:
    case TYPE_FUNCTION:
        return FE_TYPE_PTR;
    default:
        crash("can't convert mars type to iron type");
    }
    return FE_TYPE_VOID;
}