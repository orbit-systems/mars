#pragma once
#define PHOBOS_AST_H

#include "../orbit.h"
#include "lexer.h"

typedef struct {
    token* start;
    token* end;
} ast_base;


// define all the AST node macros
#define AST_NODES \
    AST_TYPE(expr_identifier, "identifier", { \
        union { \
        ast_base base; \
        token* tok; \
        }; \
    }) \
    AST_TYPE(int_lit_expr, "int literal", { \
        ast_base base; \
        token* lit; \
        u64 value; \
    }) \
    AST_TYPE(float_lit_expr, "float literal", { \
        ast_base base; \
        token* lit; \
        f64 value; \
    }) \
    AST_TYPE(string_lit_expr, "string literal", { \
        ast_base base; \
        token* lit; \
        string value; \
    }) \
    AST_TYPE(paren_expr, "parenthesis", { \
        ast_base base; \
        AST subexpr; \
    }) \
    AST_TYPE(cast_expr, "cast", { \
        ast_base base; \
        AST type; \
        AST rhs; \
        bool is_bitcast; \
    }) \
    AST_TYPE(unary_op_expr, "unary op", { \
        ast_base base; \
        token* op; \
        AST rhs; \
    }) \
    AST_TYPE(binary_op_expr, "binary op", { \
        ast_base base; \
        token* op; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(selector_expr, "selector", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(impl_selector_expr, "implicit selector", { \
        ast_base base; \
        AST rhs; \
    }) \
    AST_TYPE(array_index_expr, "array index", { \
        ast_base base; \
        AST lhs; \
        AST inside; \
    }) \
    AST_TYPE(slice_expr, "slice", { \
        ast_base base; \
        AST lhs; \
        AST inside_left; \
        AST inside_right; \
    }) \
    AST_TYPE(decl_stmt, "declaration", { \
        ast_base base; \
        AST lhs; \
        AST type; \
        AST rhs; \
        bool has_expl_type; \
        bool is_mut; \
    }) \
    AST_TYPE(type_stmt, "type declaration", { \
        ast_base base; \
        AST lhs; \
        AST type; \
        AST rhs; \
        bool has_expl_type; \
        bool is_mut; \
    }) \
    AST_TYPE(assign_stmt, "assignment", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(comp_assign_stmt, "compound assignment", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
        token* op; \
    }) \
    AST_TYPE(empty_stmt, "empty statement", { \
        union{ \
        ast_base base; \
        token* this; \
        }; \
    }) \

// generate the enum tags for the AST tagged union
typedef u16 ast_type; enum {
    astype_invalid,
#define AST_TYPE(ident, identstr, structdef) astype_##ident,
    AST_NODES
#undef AST_TYPE
    astype_COUNT,
};

// generate tagged union AST type
typedef struct {
    ast_type type;
    union {
        void* rawptr;
        ast_base * base;
#define AST_TYPE(ident, identstr, structdef) struct ast_##ident * ident;
        AST_NODES
#undef AST_TYPE
    };
} AST;

// generate AST node typedefs
#define AST_TYPE(ident, identstr, structdef) typedef struct ast_##ident structdef ast_##ident;
    AST_NODES
#undef AST_TYPE

extern char* ast_type_str[];
extern size_t ast_type_size[];