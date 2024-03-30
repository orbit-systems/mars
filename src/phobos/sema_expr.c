#include "orbit.h"
#include "mars.h"

#include "phobos.h"
#include "sema.h"

// i may in fact explode
// writing this is agony

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

forceinline type* normalize_simple_untyped_type(type* t) {
    switch (t->tag) {
    case T_UNTYPED_FLOAT: return make_type(T_F64);
    case T_UNTYPED_INT:   return make_type(T_I64);
    default:              return t;
    }
}

forceinline bool is_type_integer(type* restrict t) {
    switch (t->tag) {
    case T_UNTYPED_INT:
    case T_I8:
    case T_I16:
    case T_I32:
    case T_I64:
        return true;
    default:
        return false;
    }
}

forceinline bool is_type_uinteger(type* restrict t) {
    switch (t->tag) {
    case T_U8:
    case T_U16:
    case T_U32:
    case T_U64:
        return true;
    default:
        return false;
    }
}

forceinline bool is_pointer(type* restrict t) {
   return (t->tag == T_POINTER || t->tag == T_ADDR);
}

forceinline bool is_untyped(type* restrict t) {
    return (t->tag == T_UNTYPED_INT || t->tag == T_UNTYPED_FLOAT || t->tag == T_UNTYPED_AGGREGATE);
}

forceinline bool is_numeric(type* restrict t) {
    return (t->tag >= T_UNTYPED_INT && t->tag <= T_F64);
}

forceinline entity_table* global_et(entity_table* restrict et) {
    while (et->parent) et = et->parent;
    return et;
}

void check_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint) {
    info->expr = expr;

    // dispatch
    switch (expr.type) {
    case astype_literal_expr:    check_literal_expr   (mod, et, expr, info, must_comptime_const, NULL); break;
    case astype_identifier_expr: check_ident_expr     (mod, et, expr, info, must_comptime_const, NULL); break;
    case astype_unary_op_expr:   check_unary_op_expr  (mod, et, expr, info, must_comptime_const, NULL); break;
    case astype_binary_op_expr:  check_binary_op_expr (mod, et, expr, info, must_comptime_const, NULL); break;
    default:
        error_at_node(mod, expr, "expected value expression, got %s", ast_type_str[expr.type]);
    }
}

void check_literal_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint) {
    ast_literal_expr* restrict literal = expr.as_literal_expr;
    info->ev = &literal->value;

    info->local_ref   = false;
    info->mutable     = false;
    info->use_returns = false;

    switch (literal->value.kind) {
    case ev_bool:          info->type = make_type(T_BOOL);          break;
    case ev_untyped_float: info->type = make_type(T_UNTYPED_FLOAT); break;
    case ev_untyped_int:   info->type = make_type(T_UNTYPED_INT);   break;
    case ev_pointer:       info->type = make_type(T_ADDR);          break;
    case ev_string:        info->type = make_simple_slice(T_U8);    break;
    default:               CRASH("unhandled literal ev type");
    }
}

void check_ident_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint) {
    ast_identifier_expr* restrict ident = expr.as_identifier_expr;

    if (ident->is_discard) error_at_node(mod, expr, "_ cannot be used in expression");
    if (!ident->entity) ident->entity = search_for_entity(et, ident->tok->text);
    entity* restrict ent = ident->entity;

    if (!ent || is_null_AST(ent->decl)) error_at_node(mod, expr, "'"str_fmt"' undefined at this point", str_arg(ident->tok->text));

    if (ent->is_module) error_at_node(mod, expr, "expected value expression, got imported module");
    if (ent->is_type)   error_at_node(mod, expr, "expected value expression, got type expression");
    
    if (must_comptime_const && !ent->const_val) error_at_node(mod, expr, "expression must be compile-time constant");

    if (!info->mutable) {
        info->ev = ent->const_val;
    }
    info->addressable = true;
    info->local_derived = !is_global(ent);
    info->mutable     = ent->is_mutable;
    info->type        = ent->entity_type;
    info->local_ref   = false;
    info->use_returns = false;
    ent->is_used      = true;
}


void check_unary_op_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint) {
    ast_unary_op_expr* restrict unary = expr.as_unary_op_expr;

    checked_expr subexpr = {0};
    check_expr(mod, et, unary->inside, &subexpr, must_comptime_const, NULL);
    
    switch (unary->op->type) {
    case tt_sub: { // - numeric negative
        if (!is_numeric(subexpr.type)) {
            error_at_node(mod, unary->inside, "expected a numeric type");
        }

        if (subexpr.ev) {
            info->ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
            switch (subexpr.ev->kind) {
            case ev_untyped_int:
                info->ev->as_untyped_int = -subexpr.ev->as_untyped_int;
                info->ev->kind = ev_untyped_int;
                break;
            case ev_untyped_float:
                info->ev->as_untyped_float = -subexpr.ev->as_untyped_float;
                info->ev->kind = ev_untyped_float;
                break;
            default:
                UNREACHABLE;
                break;
            }
        }
        info->type = subexpr.type;
    } break;
    case tt_tilde: { // ~ bitwise NOT
        if (!is_numeric(subexpr.type)) {
            error_at_node(mod, unary->inside, "expected a numeric type");
        }
        if (is_untyped(subexpr.type)) {
            error_at_node(mod, unary->inside, "'~' cannot be used with an untyped literal");
        }

        if (subexpr.ev) {
            info->ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
            switch (subexpr.ev->kind) {
            case ev_u8:
            case ev_i8:  info->ev->as_i8  = ~subexpr.ev->as_i8; break;
            case ev_f16:
            case ev_u16:
            case ev_i16: info->ev->as_i16 = ~subexpr.ev->as_i16; break;
            case ev_f32:
            case ev_u32:
            case ev_i32: info->ev->as_i32 = ~subexpr.ev->as_i32; break;
            case ev_f64:
            case ev_u64:
            case ev_i64: info->ev->as_i64 = ~subexpr.ev->as_i64; break;
            default:
                UNREACHABLE;
                break;
            }
        info->ev->kind = subexpr.ev->kind;
        info->type = subexpr.type;
        }
    } break;
    case tt_exclam: { // ! boolean NOT
        if (!is_pointer(subexpr.type) && !(subexpr.type->tag != T_BOOL)) {
            error_at_node(mod, unary->inside, "expected pointer or boolean");
        }

        if (subexpr.ev) {
            info->ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
            switch (subexpr.ev->kind) {
            case ev_pointer: info->ev->as_bool = !subexpr.ev->as_pointer;
            case ev_bool:    info->ev->as_bool = !subexpr.ev->as_bool;
            default:
                UNREACHABLE;
                break;
            }
        info->ev->kind = subexpr.ev->kind;
        }

        info->type = make_type(T_BOOL);
    } break;
    case tt_and: { // & make-pointer operator
        if (!subexpr.addressable) {
            error_at_node(mod, unary->inside, "expression is not addressable");
        }

        info->type = make_type(T_POINTER);
        set_target(info->type, subexpr.type);
        info->type->as_reference.mutable = subexpr.mutable;
        

    } break;
    }
}

void check_binary_op_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint) {
    TODO("");
}

// construct a type and embed it in the type graph
type* type_from_expr(mars_module* restrict mod, entity_table* restrict et, AST expr) {

}