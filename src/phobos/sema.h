#pragma once
#define PHOBOS_CHECKER_H

#include "phobos.h"
#include "parse.h"
#include "term.h"
#include "ast.h"
#include "exactval.h"

#include "entity.h"
#include "type.h"

// emit an error that highlights an AST node
#define error_at_node(module, node, msg, ...) do { \
    string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                 \
    mars_file* source_file = find_source_file((module), ast_str);                                 \
    if (source_file == NULL) CRASH("source file not found for AST node");                         \
    error_at_string(source_file->path, source_file->src, ast_str, msg __VA_OPT__(,) __VA_ARGS__); \
} while (0)

// emit a warning that highlights an AST node
#define warning_at_node(module, node, msg, ...) do { \
    string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                   \
    mars_file* source_file = find_source_file((module), ast_str);                                   \
    if (source_file == NULL) CRASH("source file not found for AST node");                           \
    warning_at_string(source_file->path, source_file->src, ast_str, msg __VA_OPT__(,) __VA_ARGS__); \
} while (0)

void check_program(mars_module* restrict mod);
void check_module(mars_module* restrict mod);
void collect_globals(mars_module* restrict mod, entity_table* restrict et);
void collect_entites(mars_module* restrict mod, entity_table* restrict et, da(AST) stmts, bool global);

typedef struct checked_expr {
    AST expr;

    type* type;
    exact_value* ev; // for if is compile time constant
    bool use_returns : 1; // use return list of type* as the type. for functions that return multiple things

    bool mutable       : 1;
    bool local_ref     : 1; // so that we can warn against returning local pointers and shit 
    bool local_derived : 1;
    bool addressable   : 1;
} checked_expr;

void check_stmt(mars_module* restrict mod, entity_table* restrict et, AST stmt);



// pass in a checked_expr struct for check_expr to fill out
void check_expr          (mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint);
void check_ident_expr    (mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint);
void check_literal_expr  (mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint);
void check_unary_op_expr (mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint);
void check_binary_op_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint);
void check_cast_expr     (mars_module* restrict mod, entity_table* restrict et, AST expr, checked_expr* restrict info, bool must_comptime_const, type* restrict typehint);

type* type_from_expr(mars_module* restrict mod, entity_table* restrict et, AST expr, bool no_error);