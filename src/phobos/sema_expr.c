#include "orbit.h"
#include "mars.h"

#include "phobos.h"
#include "sema.h"
#include "type.h"

// i may in fact explode
// writing this is agony

static type* make_string() {
    static type* str = NULL;
    if (str != NULL) {
        return str;
    }
    str = make_type(T_SLICE);
    set_target(str, make_type(T_U8));
    return str;
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

// struct_field* get_field_properties(type* t, string query) {
//     if (query.len == 1 && *query.raw == '_') return NULL;

//     FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
//         struct_field* field = &t->as_aggregate.fields.at[i];

//         if (string_eq(query, field->name)) {
//             return field;
//         }

//         if (string_eq(field->name, to_string("_"))) {
//             struct_field* subfield = get_field_properties(field->subtype, query);
//             if (subfield != NULL) return subfield;
//         }
//     }

//     return NULL;
// }

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
    case EV_BOOL:          info->type = make_type(T_BOOL);          break;
    case EV_UNTYPED_FLOAT: info->type = make_type(T_UNTYPED_FLOAT); break;
    case EV_UNTYPED_INT:   info->type = make_type(T_UNTYPED_INT);   break;
    case EV_POINTER:       info->type = make_type(T_ADDR);          break;
    case EV_STRING:        info->type = make_string();                   break;
    default:               CRASH("unhandled literal ev type");
    }
}

void check_ident_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    ast_identifier_expr* ident = expr.as_identifier_expr;

    if (ident->is_discard) error_at_node(mod, expr, "_ cannot be used in expression");
    if (!ident->entity) ident->entity = search_for_entity(et, ident->tok->text);
    entity* ent = ident->entity;

    if (ent == NULL) error_at_node(mod, expr, "'"str_fmt"' undefined", str_arg(ident->tok->text));

    if (must_comptime_const&& ent->visited) {
        error_at_node(mod, expr, "constant expression has cyclic dependencies");
    }

    if (!ent->checked) check_stmt(mod, global_et(et), ent->decl); // on-the-fly global checking

    if (ent->is_module) error_at_node(mod, expr, "expected value expression, got imported module");
    if (ent->is_type)   error_at_node(mod, expr, "expected value expression, got type expression");

    assert(!ent->entity_type);

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
    case TT_SUB: { // - numeric negative
        if (!is_numeric(subexpr.type)) {
            error_at_node(mod, unary->inside, "expected a numeric type");
        }

        if (subexpr.ev) {
            info->ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
            switch (subexpr.ev->kind) {
            case EV_UNTYPED_INT:
                info->ev->as_untyped_int = -subexpr.ev->as_untyped_int;
                info->ev->kind = EV_UNTYPED_INT;
                break;
            case EV_UNTYPED_FLOAT:
                info->ev->as_untyped_float = -subexpr.ev->as_untyped_float;
                info->ev->kind = EV_UNTYPED_FLOAT;
                break;
            default:
                UNREACHABLE;
                break;
            }
        }
        info->type = subexpr.type;
    } break;
    case TT_TILDE: { // ~ bitwise NOT
        if (!is_numeric(subexpr.type)) {
            error_at_node(mod, unary->inside, "expected a numeric type");
        }
        if (is_untyped(subexpr.type)) {
            error_at_node(mod, unary->inside, "'~' cannot be used with an untyped literal");
        }

        if (subexpr.ev) {
            info->ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
            switch (subexpr.ev->kind) {
            case EV_U8:
            case EV_I8:  info->ev->as_i8  = (i8)~(u8)subexpr.ev->as_i8; break;
            case EV_F16:
            case EV_U16:
            case EV_I16: info->ev->as_i16 = (i16)~(u16)subexpr.ev->as_i16; break;
            case EV_F32:
            case EV_U32:
            case EV_I32: info->ev->as_i32 = (i32)~(u32)subexpr.ev->as_i32; break;
            case EV_F64:
            case EV_U64:
            case EV_I64: info->ev->as_i64 = (i64)~(u64)subexpr.ev->as_i64; break;
            default:
                UNREACHABLE;
                break;
            }
        info->ev->kind = subexpr.ev->kind;
        info->type = subexpr.type;
        }
    } break;
    case TT_EXCLAM: { // ! boolean NOT
        if (subexpr.type->tag != T_BOOL) {
            error_at_node(mod, unary->inside, "expected pointer or boolean");
        }

        if (subexpr.ev) {
            info->ev->as_bool = !subexpr.ev->as_bool;
            info->ev->kind = EV_BOOL;
            info->local_derived = subexpr.local_derived;
        }

        info->type = make_type(T_BOOL);
    } break;
    case TT_AND: { // & reference operator
        if (!subexpr.addressable) {
            error_at_node(mod, unary->inside, "expression is not addressable");
        }

        info->type = make_type(T_POINTER);
        set_target(info->type, subexpr.type);
        info->type->as_reference.mutable = subexpr.mutable;
        info->local_ref = subexpr.local_derived;
    } break;
    case TT_CARAT: { // ^ dereference operator
        if (!is_pointer(subexpr.type)) {
            error_at_node(mod, unary->inside, "expression is not dereferencable");
        }

        info->type = get_target(subexpr.type);
        info->mutable = subexpr.type->as_reference.mutable;
    } break;
    case TT_KEYWORD_OFFSETOF: { // offsetof( entity.field )
        if (!subexpr.addressable) { // cant get offset if its not addressable!
            error_at_node(mod, unary->inside, "cannot get offset of expression");
        }
        
        exact_value* ev = new_exact_value(NO_AGGREGATE, USE_MALLOC);
        ev->kind = EV_UNTYPED_INT;

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

        // struct_field* field_props = get_field_properties(struct_type, field_name);
        //assert(field_props != NULL);

        //ev->as_untyped_int = field_props->offset;

        return;
    }
    }
}

void check_binary_op_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint) {
    TODO("");
}

void fill_aggregate_offsets(type* t) {
    assert(t->tag == T_STRUCT || t->tag == T_UNION);

    TODO("fill aggregate offsets");

    if (t->tag == T_UNION) {
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            t->as_aggregate.fields.at[i].offset = 0;
        }
    } else {
        u64 running_offset = 0;
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            running_offset = align_forward(running_offset, type_real_align_of(t->as_aggregate.fields.at[i].subtype));
            t->as_aggregate.fields.at[i].offset = running_offset;
            running_offset += type_real_size_of(t->as_aggregate.fields.at[i].subtype);
        }
    }
}

// construct a type and embed it in the type graph
type* type_from_expr(mars_module* mod, entity_table* et, AST expr, bool no_error, bool top) {
    if (is_null_AST(expr)) return NULL;

    switch (expr.type) {
    case AST_paren_expr: { // ()
        return type_from_expr(mod, et, expr.as_paren_expr->subexpr, no_error, top);
    }
    case AST_basic_type_expr: { // i32, bool, addr, et cetera
        switch (expr.as_basic_type_expr->lit->type) {
        case TT_TYPE_KEYWORD_I8:    return make_type(T_I8);
        case TT_TYPE_KEYWORD_I16:   return make_type(T_I16);
        case TT_TYPE_KEYWORD_I32:   return make_type(T_I32);
        case TT_TYPE_KEYWORD_I64:
        case TT_TYPE_KEYWORD_INT:   return make_type(T_I64);
        case TT_TYPE_KEYWORD_U8:    return make_type(T_U8);
        case TT_TYPE_KEYWORD_U16:   return make_type(T_U16);
        case TT_TYPE_KEYWORD_U32:   return make_type(T_U32);
        case TT_TYPE_KEYWORD_U64:
        case TT_TYPE_KEYWORD_UINT:  return make_type(T_U64);
        case TT_TYPE_KEYWORD_F16:   return make_type(T_F16);
        case TT_TYPE_KEYWORD_F32:   return make_type(T_F32);
        case TT_TYPE_KEYWORD_F64:
        case TT_TYPE_KEYWORD_FLOAT: return make_type(T_F64);
        case TT_TYPE_KEYWORD_ADDR:  return make_type(T_ADDR);
        case TT_TYPE_KEYWORD_BOOL:  return make_type(T_BOOL);
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