#include "irgen.h"

#include "phobos/analysis/sema.h"
#include "common/ptrmap.h"

static mars_module* mars_mod;
static FeModule* am;

static PtrMap entity2stackoffset;

static i64 _bb_uID;

#define LOG(...) printf(__VA_ARGS__)

string bb_uID() {
    char* buf = mars_alloc(sizeof(buf) * 16); //block1234567890\0
    sprintf(buf, str_fmt"block%d", _bb_uID++);
    return str(buf);
}

void generate_ir_iron_from_mars(mars_module* mod, FeModule* femod) {
    //we now iterate over every tree in the AST forest, and emit IR from each
    mars_mod = mod;
    foreach(AST root, mod->program_tree) {
        printf("lowering %s to IR\n", ast_type_str[root.type]);
        generate_ir_global_from_stmt_decl(femod, root);
    }
}

FeData* generate_ir_global_from_stmt_decl(FeModule* mod, AST ast) { //FIXME: add sanity
    if (ast.type != AST_decl_stmt) crash("got %s in generate_ir_global_from_stmt_decl\n", ast_type_str[ast.type]);
    if (ast.as_decl_stmt->lhs.len != 1) crash("global decl_stmt only supports 1 lhs at the moment\n");
    if (ast.as_decl_stmt->lhs.at[0].type != AST_identifier) crash("lhs of decl_stmt should be identifier!\n");

    string identifier_text = ast.as_decl_stmt->lhs.at[0].as_identifier->tok->text;


    //we need to now figure out whats on the rhs, and parse correctly
    switch (ast.as_decl_stmt->rhs.type) {
        case AST_func_literal_expr:
            //we obtain the type from the entity table
            entity* func_literal_ent = search_for_entity(mars_mod->entities, identifier_text);
            if (!func_literal_ent) crash("expected entity "str_fmt" to exist in global scope!", str_arg(identifier_text));

            /*
            module_name.function:
                d64 module_name.function.code

            module_name.function.code:
            */
            char* buf = mars_alloc(mars_mod->module_name.len + identifier_text.len + strlen("..code") + 1);
            sprintf(buf, str_fmt"."str_fmt".code", str_arg(mars_mod->module_name), str_arg(identifier_text));
            FeSymbol* code_sym = fe_new_symbol(mod, str(buf), FE_BIND_LOCAL);

            buf = mars_alloc(mars_mod->module_name.len + identifier_text.len + strlen("..") + 1);
            sprintf(buf, str_fmt"."str_fmt, str_arg(mars_mod->module_name), str_arg(identifier_text));
            FeSymbol* data_sym = fe_new_symbol(mod, str(buf), FE_BIND_EXPORT);
            FeData* datum = fe_new_data(mod, data_sym, true);
            fe_set_data_symref(datum, code_sym);

            FeFunction* func = fe_new_function(mod, code_sym);
            foreach(Type* param, func_literal_ent->entity_type->as_function.params)  
            fe_add_func_param(func, TEType_to_iron(mod, param));
            foreach(Type* ret, func_literal_ent->entity_type->as_function.returns) 
            fe_add_func_return(func, TEType_to_iron(mod, ret));
            //finished with the function now, we can now create BB

            FeBasicBlock* bb = fe_new_basic_block(func, bb_uID());

            bb = generate_ir_stmt_block(mod, bb, ast.as_decl_stmt->rhs.as_func_literal_expr->code_block);

            return datum;
            
        default:
            crash("unexpected rhs: %s\n", ast_type_str[ast.as_decl_stmt->rhs.type]);
    }
}

FeBasicBlock* generate_ir_stmt_block(FeModule* mod, FeBasicBlock* bb, AST stmt_block) {
    if (stmt_block.type != AST_stmt_block) crash("expected AST_stmt_block, got %s\n", ast_type_str[stmt_block.type]);

    foreach(AST stmt, stmt_block.as_stmt_block->stmts) {
        switch (stmt.type) {
            default:
                crash("unexpected ast type: %s\n", ast_type_str[stmt.type]);
        }
    }
}

FeType* TEType_to_iron(FeModule* mod, Type* t) {
    switch (t->tag) {
        case TYPE_I8:
        case TYPE_U8:
            return fe_type(mod, FE_TYPE_I8);
        case TYPE_I16:
        case TYPE_U16:
            return fe_type(mod, FE_TYPE_I16);
        case TYPE_I32:
        case TYPE_U32:
            return fe_type(mod, FE_TYPE_I32);
        case TYPE_I64:
        case TYPE_U64:
            return fe_type(mod, FE_TYPE_I64);
        default:
            crash("unexpected type tag: %d\n", t->tag);
    }
}

//FIXME: add generate_ir_local_from_stmt_decl



FeInst* generate_ir_expr_literal(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_binop(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_ident_load(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_value(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeInst* generate_ir_expr_address(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

void generate_ir_stmt_assign(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

void generate_ir_stmt_return(FeFunction* f, FeBasicBlock* bb, AST ast) {
    TODO("");
}

FeFunction* generate_ir_function(FeModule* mod, AST ast) {
    TODO("");
}

FeType* translate_type(FeModule* m, Type* t) {
    TODO("");
}