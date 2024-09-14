#include "irgen.h"

#include "phobos/analysis/sema.h"
#include "common/ptrmap.h"

static mars_module* mars_mod;
static FeModule* am;

static PtrMap entity2stackoffset;

static i64 _bb_uID = 0;

#define LOG(...) printf(__VA_ARGS__)

string bb_uID() {
    char* buf = mars_alloc(sizeof(buf) * 16); //block1234567890\0
    sprintf(buf, "block%d", _bb_uID++);
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
            printf("TODO: if function has named returns, generate frames for the returns\n");
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

            FeBasicBlock* bb = fe_new_basic_block(func, bb_uID());
            //we now fill out prologue info for the BB, like paramvals while adding params

            fe_init_func_params(func, 1);
            fe_init_func_returns(func, 1);

            foreach(Type* param, func_literal_ent->entity_type->as_function.params) {  
                fe_add_func_param(func, TEType_to_iron(mod, param));
                fe_append(bb, fe_inst_paramval(func, count));
            }

            foreach(Type* ret, func_literal_ent->entity_type->as_function.returns) 
                fe_add_func_return(func, TEType_to_iron(mod, ret));
            //finished with the function now, we can now create BB

            bb = generate_ir_stmt_block(func, bb, ast.as_decl_stmt->rhs.as_func_literal_expr->code_block);

            string s = fe_emit_ir(mod, true);
            printf(str_fmt, str_arg(s));
            return datum;
            
        default:
            crash("unexpected rhs: %s\n", ast_type_str[ast.as_decl_stmt->rhs.type]);
    }
    return NULL;
}

FeBasicBlock* generate_ir_stmt_block(FeFunction* func, FeBasicBlock* bb, AST stmt_block) {
    if (stmt_block.type != AST_stmt_block) crash("expected AST_stmt_block, got %s\n", ast_type_str[stmt_block.type]);

    foreach(AST stmt, stmt_block.as_stmt_block->stmts) {
        switch (stmt.type) {
            case AST_return_stmt:
                //we need to generate code for the return expr first, and then generate the return IR
                foreach(AST ret, stmt.as_return_stmt->returns) {
                    FeInst* ret_val = generate_ir_expr(func, bb, ret);
                    fe_append(bb, fe_inst_returnval(func, count, ret_val));
                }
                fe_append(bb, fe_inst_return(func));
                return bb;

            default:
                crash("unexpected ast type: %s\n", ast_type_str[stmt.type]);
        }
    }
    return NULL;
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
    return NULL;
}

//FIXME: add generate_ir_local_from_stmt_decl

FeInst* generate_ir_expr(FeFunction* func, FeBasicBlock* bb, AST ast) {
    //auauauauagh
    switch(ast.type) {
        case AST_binary_op_expr:
            FeInst* lhs = generate_ir_expr(func, bb, ast.as_binary_op_expr->lhs);
            FeInst* rhs = generate_ir_expr(func, bb, ast.as_binary_op_expr->rhs);
            //we now need to turn op str into op inst
            int inst_type = FE_INST_INVALID;
            if (string_eq(str("+"), ast.as_binary_op_expr->op->text)) inst_type = FE_INST_ADD;
            else crash("unknown op: "str_fmt"\n", str_arg(ast.as_binary_op_expr->op->text));
            return fe_append(bb, fe_inst_binop(func, inst_type, lhs, rhs));
        case AST_identifier:
            //we need to see if this identifier is a param or a return!
            entity* ident_ent = ast.as_identifier->entity;
            printf("identifier: "str_fmt"\n", str_arg(ast.as_identifier->tok->text));

            //got the entity, now we need to figure out what this identifier means
            if (!ident_ent->is_param) crash("we only handle identifiers as parameters right meow\n");
            //we return FeInst* for the corresponding paramval
            return bb->at[ident_ent->param_idx];
        default:
            crash("unexpected ast type: %s\n", ast_type_str[ast.type]);
    }
    return NULL;
}

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