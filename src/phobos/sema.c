#include "orbit.h"
#include "mars.h"

#include "phobos.h"
#include "sema.h"

void check_module_and_dependencies(mars_module* restrict mod) {
    FOR_URANGE(i, 0, mod->import_list.len) {
        if (!mod->import_list.at[i]->checked) {
            check_module_and_dependencies(mod->import_list.at[i]);
        }
    }
    // all imported modules have been checked
    check_module(mod);
}

// assumes imports have all been checked
void check_module(mars_module* restrict mod) {
    // printf("checking module "str_fmt"\n", str_arg(mod->module_name));

    // TODO use entities from imported modules
    entity_table* globals = new_entity_table(NULL);
    collect_globals(mod, globals);

    FOR_URANGE(i, 0, globals->len) {
        if (!globals->at[i]->entity_type) {
            // check_global_decl();
        }
    }

    mod->checked = true;
}

void collect_globals(mars_module* restrict mod, entity_table* restrict et) {
    collect_entites(mod, et, mod->program_tree, true);
}

void collect_decl(mars_module* mod, entity_table* et, AST stmt) {
    if (stmt.type == AST_decl_stmt) {
        ast_decl_stmt* decl = stmt.as_decl_stmt;
        FOR_URANGE(j, 0, decl->lhs.len) {
            if (decl->lhs.at[j].type != AST_identifier_expr) {
                error_at_node(mod, decl->lhs.at[j], "entity must be an identifier");
            }

            ast_identifier_expr* ident_expr = decl->lhs.at[j].as_identifier_expr;
            if (ident_expr->is_discard) continue;

            string ident = ident_expr->tok->text;
            entity* e = search_for_entity(et, ident);
            if (e == NULL) {
                e = new_entity(et, ident, stmt);
            }
            if (e->decl.rawptr != stmt.rawptr) {
                error_at_node(mod, decl->lhs.at[j], "'"str_fmt"' already declared", str_arg(ident));
            }
            e->is_mutable = decl->is_mut;
            ident_expr->entity = e;
        }
    } else if (stmt.type == AST_type_decl_stmt) {
        ast_type_decl_stmt* decl = stmt.as_type_decl_stmt;
        if (decl->lhs.type != AST_identifier_expr) {
            error_at_node(mod, decl->lhs, "type entity must be an identifier");
        }
        ast_identifier_expr* ident_expr = decl->lhs.as_identifier_expr;
        if (ident_expr->is_discard) return;

        string ident = ident_expr->tok->text;
        entity* e = search_for_entity(et, ident);
        if (e == NULL) {
            e = new_entity(et, ident, stmt);
        }
        if (e->decl.rawptr != stmt.rawptr) {
            error_at_node(mod, decl->lhs, "'"str_fmt"' already declared", str_arg(ident));
        }
        if (e->entity_type == NULL) {
            e->entity_type = make_type(T_ALIAS);
        }
        e->is_type = true;
        ident_expr->entity = e;
    } else {
        error_at_node(mod, stmt, "expected let, mut, or type declaration");
    }
}

void collect_entites(mars_module* restrict mod, entity_table* restrict et, da(AST) stmts, bool global) {
    
    FOR_URANGE(i, 0, stmts.len) {
        switch (stmts.at[i].type){
        case AST_decl_stmt:
        case AST_type_decl_stmt:
            collect_decl(mod, et, stmts.at[i]);
            break;
        case AST_extern_stmt: {
            if (!global) {
                error_at_node(mod, stmts.at[i], "extern statements only allowed in global scope");
            }
            if (stmts.at[i].as_extern_stmt->decl.type == AST_block_stmt) {
                ast_block_stmt* blockstmt = stmts.at[i].as_extern_stmt->decl.as_block_stmt;
                FOR_URANGE(j, 0, blockstmt->stmts.len) {
                    AST decl = blockstmt->stmts.at[j];
                    if (decl.type == AST_decl_stmt) {
                        collect_decl(mod, et, decl);
                    } else if (decl.type == AST_empty_stmt) {
                        // do nothing lmao
                    } else {
                        error_at_node(mod, decl, "expected a let or mut declaration");
                    }
                }
            } else if (stmts.at[i].as_extern_stmt->decl.type == AST_block_stmt) {
                collect_decl(mod, et, stmts.at[i].as_extern_stmt->decl);
            } else {
                error_at_node(mod, stmts.at[i].as_extern_stmt->decl, "expected a let or mut declaration");
            }
        } break;
        case AST_import_stmt: {
            if (!global) {
                error_at_node(mod, stmts.at[i], "import statements only allowed in global scope");
            }

            ast_import_stmt* import = stmts.at[i].as_import_stmt;

            string ident = import->name.as_identifier_expr->tok->text;
            entity* e = search_for_entity(et, ident);
            if (e == NULL) {
                e = new_entity(et, ident, stmts.at[i]);
            }
            if (e->decl.rawptr != stmts.at[i].rawptr) {
                error_at_node(mod, import->name, "'"str_fmt"' already declared", str_arg(ident));
            }
            FOR_URANGE(j, 0, mod->import_list.len) {
                if (string_eq(mod->import_list.at[j]->module_path, import->realpath)) {
                    e->is_module = true;
                    e->module = mod->import_list.at[j];
                }
            }
            if (!e->is_module) {
                CRASH("FUCK");
            }
        } break;
        case AST_empty_stmt:
            break;
        default:
            if (global) error_at_node(mod, stmts.at[i], "%s not allowed in global scope", ast_type_str[stmts.at[i].type]);
            break;
        }
    }
}