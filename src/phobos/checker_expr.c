#include "orbit.h"
#include "mars.h"

#include "phobos.h"
#include "checker.h"

static type* make_simple_slice(u8 tag) {
    static type* slice[T_meta_INTEGRAL] = {NULL};

    if (tag > T_meta_INTEGRAL) {
        return NULL;
    }
    if (slice[tag] != NULL) {
        return slice[tag];
    }
    slice[tag] = make_type(T_SLICE);
    set_target(slice[tag], make_type(tag));
    return slice[tag];
}

type* normalize_simple_untyped_type(type* t) {
    switch (t->tag) {
    case T_UNTYPED_FLOAT: return make_type(T_F64);
    case T_UNTYPED_INT:   return make_type(T_I64);
    default:              return t;
    }
}

void check_expr(
    mars_module* restrict mod, 
    entity_table* restrict et, 
    AST expr, 
    checked_expr* restrict info, 
    bool must_comptime_const)
{
    info->expr = expr;

    switch (expr.type) {
    case astype_literal_expr: {
        ast_literal_expr* restrict literal = expr.as_literal_expr;
        info->ev = &literal->value;

        info->is_type     = false;
        info->local_ref   = false;
        info->mutable     = false;
        info->use_returns = false;

        switch (literal->value.kind) {
        case ev_bool:    info->type = make_type(T_BOOL);          break;
        case ev_float:   info->type = make_type(T_UNTYPED_FLOAT); break;
        case ev_int:     info->type = make_type(T_UNTYPED_INT);   break;
        case ev_pointer: info->type = make_type(T_ADDR);          break;
        case ev_string:  info->type = make_simple_slice(T_U8);    break;
        default:         CRASH("unhandled literal ev type");
        }

    } break;
    case astype_comp_literal_expr: {
        ast_comp_literal_expr* restrict literal = expr.as_comp_literal_expr;
        exact_value* restrict ev = new_exact_value(literal->elems.len, USE_MALLOC);
        
        TODO("check compound literal expressions");

    } break;
    case astype_identifier_expr: {
        ast_identifier_expr* restrict ident = expr.as_identifier_expr;
        if (ident->is_discard) error_at_node(mod, expr, "_ cannot be used in expression");
        if (!ident->entity) ident->entity = search_for_entity(et, ident->tok->text);
        entity* restrict ent = ident->entity;
        if (!ent || is_null_AST(ent->decl)) error_at_node(mod, expr, "'"str_fmt"' undefined at this point", str_arg(ident->tok->text));
        if (ent->is_module) error_at_node(mod, expr, "expected value expression, got imported module");
        if (ent->is_type) error_at_node(mod, expr, "expected value expression, got type expression");

        if (must_comptime_const && !ent->const_val) {
            error_at_node(mod, expr, "expression must be compile-time constant");
        }

        if (!ent->entity_type) {
            CRASH("entity should be checked before its references are checked, but it is not for some reason");
        }

        ent->is_used = true;
        info->mutable = ent->is_mutable;
        if (!info->mutable) info->ev = ent->const_val;
        info->type    = ent->entity_type;
        info->local_ref = false;
        info->use_returns = false;

    } break;
    default:
        error_at_node(mod, expr, "expected value expression, got %s", ast_type_str[expr.type]);
    }
}