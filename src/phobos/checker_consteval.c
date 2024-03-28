#include "orbit.h"
#include "mars.h"

#include "phobos.h"
#include "checker.h"

bool attempt_consteval(mars_module* restrict mod, entity_table* restrict et, AST expr, exact_value** restrict ev) {
    switch (expr.type) {
    case astype_identifier_expr: {
        ast_identifier_expr* ident = expr.as_identifier_expr;
        if (ident->entity == NULL)      error_at_node(mod, expr, "'"str_fmt"' undefined", str_arg(expr.as_identifier_expr->tok->text));
        if (ident->is_discard)          error_at_node(mod, expr, "cannot use _ in expression");
        if (ident->entity->is_type)     error_at_node(mod, expr, "expected value expression, got type");
        if (ident->entity->is_mutable)  return false;
        if (ident->entity->const_val)   {
            *ev = ident->entity->const_val;
            return true;
        }

        TODO("consteval derive from other entities");

        } break;
    default:
    }
    error_at_node(mod, expr, "expected value expression, got %s", ast_type_str[expr.type]);

}