#include "common/orbit.h"
#include "mars/mars.h"

#include "sema.h"
#include "ast.h"

void check_stmt(mars_module* mod, entity_table* et, ast_func_literal_expr* fn, AST stmt, bool global) {

    switch (stmt.type) {
    case AST_decl_stmt:
        check_decl_stmt(mod, et, fn, stmt, global);
        break;
    case AST_return_stmt:
        check_return_stmt(mod, et, fn, stmt);
        break;
    case AST_block_stmt:
        for_urange(i, 0, stmt.as_block_stmt->stmts.len) {
            check_stmt(mod, et, fn, stmt.as_block_stmt->stmts.at[i], global);
        }
        break;
    default:
        warning_at_node(mod, stmt, "unhandled '%s'", ast_type_str[stmt.type]);
        break;
    }

    // TODO("check_stmt");
}

void check_decl_stmt(mars_module* mod, entity_table* et, ast_func_literal_expr* fn, AST stmt, bool global) {
    ast_decl_stmt* decl = stmt.as_decl_stmt;

    if (decl->lhs.len != 1) {
        warning_at_node(mod, stmt, "cannot check multi-decls at the moment");
        CRASH("cannot check multi-decls at the moment");
    }

    assert(decl->lhs.at[0].type == AST_identifier_expr);
    string ident = decl->lhs.at[0].as_identifier_expr->tok->text;
    
    entity* e = new_entity(et, ident, stmt);
    e->is_mutable = decl->is_mut;
    if (decl->has_expl_type || !is_null_AST(decl->type)) {
        e->entity_type = type_from_expr(mod, et, decl->type, false, true);
    }

    if (global && is_null_AST(decl->rhs) && !decl->is_mut && !decl->is_uninit) {
        error_at_node(mod, stmt, "global let declaration must be initialized with a value");
    }

    checked_expr body = {0};
    assert(!is_null_AST(decl->rhs));
    check_expr(mod, et, decl->rhs, &body, global, NULL);
}

void check_return_stmt(mars_module* mod, entity_table* et, ast_func_literal_expr* fn, AST stmt) {
    ast_return_stmt* ret = stmt.as_return_stmt;
    
    bool is_simple_return = fn->type.as_fn_type_expr->simple_return;
    if (is_simple_return && ret->returns.len != 1) {
        error_at_node(mod, stmt, "number of returns must be one");
    }
    if (!is_simple_return && ret->returns.len != fn->returnlen) {
        error_at_node(mod, stmt, "number of returns must be zero or %d", fn->returnlen);
    }

    for_urange(i, 0, ret->returns.len) {
        checked_expr info = {0};
        check_expr(mod, et, ret->returns.at[i], &info, false, NULL);
    }
}