#pragma once
#define PHOBOS_CHECKER_H

#include "mars/phobos/phobos.h"
#include "mars/phobos/parse/parse.h"
#include "mars/phobos/ast.h"
#include "exactval.h"
#include "entity.h"
#include "type.h"
#include "mars/term.h"
#include "common/crash.h"

// emit an error that highlights an AST node
#define error_at_node(module, node, msg, ...)                                                          \
    do {                                                                                               \
        string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                  \
        mars_file* source_file = find_source_file((module), ast_str);                                  \
        if (source_file == NULL) CRASH("source file not found for AST node");                          \
        error_at_string(source_file->path, source_file->src, ast_str, msg __VA_OPT__(, ) __VA_ARGS__); \
    } while (0)

// emit a warning that highlights an AST node
#define warning_at_node(module, node, msg, ...)                                                          \
    do {                                                                                                 \
        string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                    \
        mars_file* source_file = find_source_file((module), ast_str);                                    \
        if (source_file == NULL) CRASH("source file not found for AST node");                            \
        warning_at_string(source_file->path, source_file->src, ast_str, msg __VA_OPT__(, ) __VA_ARGS__); \
    } while (0)

typedef struct checked_expr {
    AST expr;

    Type* type;
    exact_value* ev;      // for if is compile time constant
    bool use_returns : 1; // use return list of type* as the type. for functions that return multiple things

    bool mutable : 1;
    bool addressable : 1;
    bool local_ref : 1; // so that we can warn against returning local pointers and shit
    bool local_derived : 1;
} checked_expr;

da_typedef(checked_expr);

void check_module(mars_module* mod);
Type* check_stmt(mars_module* mod, AST node, entity_table* scope);
Type* check_func_literal(mars_module* mod, AST func_literal, entity_table* scope);
checked_expr check_expr(mars_module* mod, AST node, entity_table* scope);
checked_expr check_literal(mars_module* mod, AST literal);

Type* ast_to_type(mars_module* mod, AST node);
Type* operation_to_type(token* tok);
bool check_type_cast_implicit(Type* lhs, Type* rhs);
bool check_type_cast_explicit(Type* lhs, Type* rhs);
bool check_assign_op(token* op, Type* lhs, Type* rhs);
Type* sema_type_unalias(Type* t);
bool is_integral(Type* t);

/*
void check_stmt(mars_module* mod, entity_table* et, ast_func_literal_expr* fn, AST stmt, bool global);
void check_decl_stmt(mars_module* mod, entity_table* et, ast_func_literal_expr* fn, AST stmt, bool global);
void check_return_stmt(mars_module* mod, entity_table* et, ast_func_literal_expr* fn, AST stmt);

// pass in a checked_expr struct for check_expr to fill out
void check_expr          (mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_ident_expr    (mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_literal_expr  (mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_unary_op_expr (mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_binary_op_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_cast_expr     (mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);
void check_fn_literal_expr(mars_module* mod, entity_table* et, AST expr, checked_expr* info, bool must_comptime_const, type* typehint);

type* type_from_expr(mars_module* mod, entity_table* et, AST expr, bool no_error, bool top);*/