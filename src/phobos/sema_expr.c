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

forceinline bool is_type_integer(type* t) {
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

forceinline bool is_type_uinteger(type* t) {
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

forceinline bool is_pointer(type* t) {
   return (t->tag == T_POINTER || t->tag == T_ADDR);
}

forceinline bool is_untyped(type* t) {
    return (t->tag == T_UNTYPED_INT || t->tag == T_UNTYPED_FLOAT || t->tag == T_UNTYPED_AGGREGATE);
}

forceinline bool is_numeric(type* t) {
    return (t->tag >= T_UNTYPED_INT && t->tag <= T_F64);
}

forceinline entity_table* global_et(entity_table* et) {
    while (et->parent) et = et->parent;
    return et;
}

forceinline bool is_global(entity* e) {
    return (e->top == global_et(e->top));
}

struct_field* get_field_properties(type* t, string query) {
    if (query.len == 1 && *query.raw == '_') return NULL;

    FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
        struct_field* field = &t->as_aggregate.fields.at[i];

        if (string_eq(query, field->name)) {
            return field;
        }

        if (string_eq(field->name, to_string("_"))) {
            struct_field* subfield = get_field_properties(field->subtype, query);
            if (subfield != NULL) return subfield;
        }
    }

    return NULL;
}

void check_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    info->expr = expr;

    // dispatch
    switch (expr.type) {
    case AST_paren_expr:      check_expr(mod, et, expr.as_paren_expr->subexpr, info, must_comptime_const, typehint); break;
    
    case AST_literal_expr:    check_literal_expr   (mod, et, expr, info, must_comptime_const, typehint); break;
    case AST_identifier_expr: check_ident_expr     (mod, et, expr, info, must_comptime_const, typehint); break;
    case AST_unary_op_expr:   check_unary_op_expr  (mod, et, expr, info, must_comptime_const, typehint); break;
    case AST_binary_op_expr:  check_binary_op_expr (mod, et, expr, info, must_comptime_const, typehint); break;
    default:
        error_at_node(mod, expr, "expected value expression, got %s", ast_type_str[expr.type]);
    }
}

void check_literal_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    ast_literal_expr* literal = expr.as_literal_expr;
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

void check_ident_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    ast_identifier_expr* ident = expr.as_identifier_expr;

    if (ident->is_discard) error_at_node(mod, expr, "_ cannot be used in expression");
    if (!ident->entity) ident->entity = search_for_entity(et, ident->tok->text);
    entity* ent = ident->entity;

    if (!ent || is_null_AST(ent->decl)) error_at_node(mod, expr, "'"str_fmt"' undefined", str_arg(ident->tok->text));

    if (must_comptime_const&& ent->visited) {
        error_at_node(mod, expr, "constant expression has cyclic dependencies");
    }

    if (!ent->checked) check_stmt(mod, global_et(et), ent->decl); // on-the-fly global checking

    if (ent->is_module) error_at_node(mod, expr, "expected value expression, got imported module");
    if (ent->is_type)   error_at_node(mod, expr, "expected value expression, got type expression");

    if (!ent->entity_type) error_at_node(mod, expr, "(CRASH) cannot determine type of '"str_fmt"'", str_arg(ident->tok->text));

    if (must_comptime_const && !ent->const_val) error_at_node(mod, expr, "expression must be compile-time constant");

    if (!info->mutable) {
        info->ev = ent->const_val;
    }
    info->addressable   = true;
    info->local_derived = !is_global(ent);
    info->mutable       = ent->is_mutable;
    info->type          = ent->entity_type;
    info->local_ref     = false;
    info->use_returns   = false;
    ent->is_used        = true;
}


void check_unary_op_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    ast_unary_op_expr* unary = expr.as_unary_op_expr;

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
        info->local_derived = subexpr.local_derived;
        }

        info->type = make_type(T_BOOL);
    } break;
    case tt_and: { // & reference operator
        if (!subexpr.addressable) {
            error_at_node(mod, unary->inside, "expression is not addressable");
        }

        info->type = make_type(T_POINTER);
        set_target(info->type, subexpr.type);
        info->type->as_reference.mutable = subexpr.mutable;
        info->local_ref = subexpr.local_derived;
    } break;
    case tt_carat: { // ^ dereference operator
        if (!is_pointer(subexpr.type)) {
            error_at_node(mod, unary->inside, "expression is not dereferencable");
        }

        info->type = get_target(subexpr.type);
        info->mutable = subexpr.type->as_reference.mutable;
    } break;
    case tt_keyword_offsetof: { // offsetof( entity.field )
        if (!subexpr.addressable) { // cant get offset if its not addressable!
            error_at_node(mod, unary->inside, "cannot get offset of expression");
        }
        
        exact_value* ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
        ev->kind = ev_untyped_int;

        if (unary->inside.type != AST_selector_expr) {
            warning_at_node(mod, unary->inside, "offset of non-struct-selector is zero");
            ev->as_untyped_int = 0;
            break;
        }

        assert(unary->inside.as_selector_expr->rhs.type == AST_identifier_expr);
        string field_name = unary->inside.as_selector_expr->rhs.as_identifier_expr->tok->text;

        checked_expr selectee = {0};
        check_expr(mod, et, unary->inside, &selectee, false, NULL);

        type* struct_type = selectee.type;
        assert(struct_type != NULL);

        struct_field* field_props = get_field_properties(struct_type, field_name);
        assert(field_props != NULL);

        ev->as_untyped_int = field_props->offset;


        // TODO("the rest of offsetof");
    }
    }
}

void check_binary_op_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    TODO("");
}

// construct a type and embed it in the type graph
type* type_from_expr(mars_module* mod, entity_table* et, AST expr, bool no_error, bool top) {
    if (is_null_AST(expr)) return NULL;

    switch (expr.type) {
    case AST_paren_expr: { // ()
        return type_from_expr(mod, et, expr.as_paren_expr->subexpr, no_error, true);
    }
    case AST_basic_type_expr: { // i32, bool, addr, et cetera
        switch (expr.as_basic_type_expr->lit->type) {
        case tt_type_keyword_i8:    return make_type(T_I8);
        case tt_type_keyword_i16:   return make_type(T_I16);
        case tt_type_keyword_i32:   return make_type(T_I32);
        case tt_type_keyword_i64:
        case tt_type_keyword_int:   return make_type(T_I64);
        case tt_type_keyword_u8:    return make_type(T_U8);
        case tt_type_keyword_u16:   return make_type(T_U16);
        case tt_type_keyword_u32:   return make_type(T_U32);
        case tt_type_keyword_u64:
        case tt_type_keyword_uint:  return make_type(T_U64);
        case tt_type_keyword_f16:   return make_type(T_F16);
        case tt_type_keyword_f32:   return make_type(T_F32);
        case tt_type_keyword_f64:
        case tt_type_keyword_float: return make_type(T_F64);
        case tt_type_keyword_addr:  return make_type(T_ADDR);
        case tt_type_keyword_bool:  return make_type(T_BOOL);        
        }
    } break;
    case AST_struct_type_expr: { TODO("struct types"); } break;
    case AST_union_type_expr: { TODO("union types"); } break;
    case AST_fn_type_expr: {TODO("fn types"); } break;
    case AST_pointer_type_expr: { // ^let T, ^mut T
        type* ptr = make_type(T_POINTER);
        type* subtype = type_from_expr(mod, et, expr.as_pointer_type_expr->subexpr, no_error, false);
        if (subtype == NULL) return NULL;
        ptr->as_reference.mutable = expr.as_pointer_type_expr->mutable;
        set_target(ptr, subtype);
        if (top) canonicalize_type_graph();
        return ptr;
    } break;
    case AST_slice_type_expr: { // []let T, []mut T
        type* ptr = make_type(T_SLICE);
        type* subtype = type_from_expr(mod, et, expr.as_slice_type_expr->subexpr, no_error, false);
        if (subtype == NULL) return NULL;
        ptr->as_reference.mutable = expr.as_slice_type_expr->mutable;
        set_target(ptr, subtype);
        if (top) canonicalize_type_graph();
        return ptr;
    } break;
    case AST_distinct_type_expr: { // disctinct T
        type* distinct = make_type(T_DISTINCT);
        type* subtype = type_from_expr(mod, et, expr.as_distinct_type_expr->subexpr, no_error, false);
        if (subtype == NULL) return NULL;
        set_target(distinct, subtype);
        if (top) canonicalize_type_graph();
        return distinct;
    } break;
    case AST_identifier_expr: { // T
        entity* ent = search_for_entity(et, expr.as_identifier_expr->tok->text);
        if (ent == NULL) {
            if (no_error) return NULL;
            else error_at_node(mod, expr, "'"str_fmt"' undefined", str_arg(expr.as_identifier_expr->tok->text));
        }

        if (!ent->checked) check_stmt(mod, global_et(et), ent->decl); // on-the-fly global checking

        if (!ent->is_type) {
            if (no_error) return NULL;
            else error_at_node(mod, expr, "'"str_fmt"' is not a type", str_arg(expr.as_identifier_expr->tok->text));
        }
        return ent->entity_type;
    } break;
    default:
        if (no_error) return NULL;
        else error_at_node(mod, expr, "expected type expression");
        break;
    }
}