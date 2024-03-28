#pragma once
#define PHOBOS_AST_H

#include "orbit.h"
#include "lexer.h"
#include "arena.h"
#include "exactval.h"

typedef struct {
    token* start;
    token* end;
} ast_base;

// define all the AST node macros
#define AST_NODES \
    AST_TYPE(identifier_expr, "identifier", { \
        union { \
        ast_base base; \
        token* tok; \
        }; \
        struct entity* entity; \
        bool is_discard : 1; \
    }) \
    AST_TYPE(literal_expr, "literal", { \
        ast_base base; \
        exact_value value; \
    }) \
    AST_TYPE(comp_literal_expr, "compound literal", { \
        ast_base base; \
        AST type; \
        da(AST) elems; \
    }) \
    AST_TYPE(func_literal_expr, "function literal", { \
        ast_base base; \
        AST type; \
        AST code_block; \
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
        AST inside; \
    }) \
    AST_TYPE(binary_op_expr, "binary op", { \
        ast_base base; \
        token* op; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(entity_selector_expr, "entity selector", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(selector_expr, "selector", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(return_selector_expr, "return selector", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(impl_selector_expr, "implicit selector", { \
        ast_base base; \
        AST rhs; \
    }) \
    AST_TYPE(index_expr, "array index", { \
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
    AST_TYPE(call_expr, "call", { \
        ast_base base; \
        AST lhs; \
        da(AST) params; \
    }) \
    \
    \
    AST_TYPE(module_decl, "module declaration", { \
        ast_base base; \
        token* name; \
    }) \
    AST_TYPE(import_stmt, "import statement", { \
        ast_base base; \
        AST name; \
        AST path; \
        \
        string realpath; \
    }) \
    AST_TYPE(block_stmt, "statement block", { \
        ast_base base; \
        da(AST) stmts; \
    }) \
    AST_TYPE(decl_stmt, "declaration", { \
        ast_base base; \
        da(AST) lhs; \
        AST rhs; \
        AST type; \
        bool has_expl_type : 1; \
        bool is_mut        : 1; \
        bool is_static     : 1; \
        bool is_volatile   : 1; \
        bool is_uninit     : 1; \
    }) \
    AST_TYPE(type_decl_stmt, "type declaration", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
    }) \
    AST_TYPE(assign_stmt, "assignment", { \
        ast_base base; \
        da(AST) lhs; \
        AST rhs; \
    }) \
    AST_TYPE(comp_assign_stmt, "compound assignment", { \
        ast_base base; \
        AST lhs; \
        AST rhs; \
        token* op; \
    }) \
    AST_TYPE(if_stmt, "if statement", { \
        ast_base base; \
        AST condition; \
        AST if_branch; \
        AST else_branch; \
        bool is_elif; \
    }) \
    AST_TYPE(switch_stmt, "switch statement", { \
        ast_base base; \
        AST expr; \
        da(AST) cases; \
    }) \
    AST_TYPE(case, "case statement", { \
        ast_base base; \
        da(AST) matches; \
        AST block; \
    }) \
    AST_TYPE(while_stmt, "while loop", { \
        ast_base base; \
        AST condition; \
        AST block; \
    }) \
    AST_TYPE(for_stmt, "for loop", { \
        ast_base base; \
        AST prelude; \
        AST condition; \
        AST update; \
        AST block; \
    }) \
    AST_TYPE(for_in_stmt, "for-in loop", { \
        ast_base base; \
        AST indexvar; \
        AST type; \
        AST to; \
        AST from; \
        AST block; \
        bool is_inclusive; \
        bool is_reverse; \
    }) \
    AST_TYPE(extern_stmt, "extern statement", { \
        ast_base base; \
        AST decl; \
    }) \
    AST_TYPE(defer_stmt, "defer statement", { \
        ast_base base; \
        AST stmt; \
    }) \
    AST_TYPE(expr_stmt, "expression statement", { \
        ast_base base; \
        AST expression; \
    }) \
    AST_TYPE(return_stmt, "return statement", { \
        ast_base base; \
        da(AST) returns; \
    }) \
    AST_TYPE(break_stmt, "break statement", { \
        ast_base base; \
        AST label; \
    }) \
    AST_TYPE(continue_stmt, "continue statement", { \
        ast_base base; \
        AST label; \
    }) \
    AST_TYPE(fallthrough_stmt, "fallthrough statement", { \
        ast_base base; \
    }) \
    AST_TYPE(empty_stmt, "empty statement", { \
        union{ \
        ast_base base; \
        token* tok; \
        }; \
    }) \
    AST_TYPE(label_stmt, "label", { \
        ast_base base; \
        AST label; \
    }) \
    \
    \
    \
    \
    AST_TYPE(basic_type_expr, "basic type literal", { \
        union { \
            ast_base base; \
            token* lit; \
        }; \
    }) \
    AST_TYPE(struct_type_expr, "struct type", { \
            ast_base base; \
            da(AST_typed_field) fields; \
            bool smart_pack;\
    }) \
    AST_TYPE(union_type_expr, "union type", { \
            ast_base base; \
            da(AST_typed_field) fields; \
    }) \
    AST_TYPE(fn_type_expr, "fn type", { \
            ast_base base; \
            da(AST_typed_field) parameters; \
            da(AST_typed_field) returns; \
            AST block_symbol_override; /*this will be a string literal or NULL_STR if not set*/ \
            bool always_inline;\
            bool simple_return; \
    }) \
    AST_TYPE(enum_type_expr, "enum type", { \
            ast_base base; \
            AST backing_type;\
            da(AST_enum_variant) variants; \
    }) \
    AST_TYPE(array_type_expr, "array type", { \
            ast_base base; \
            AST subexpr; \
            AST length; \
    }) \
    AST_TYPE(slice_type_expr, "slice type", { \
            ast_base base; \
            AST subexpr; \
            bool mutable; \
    }) \
    AST_TYPE(pointer_type_expr, "pointer type", { \
            ast_base base; \
            AST subexpr; \
            bool mutable; \
    }) \
    AST_TYPE(distinct_type_expr, "distinct type", { \
            ast_base base; \
            AST subexpr; \
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
typedef struct AST {
    union {
        void* rawptr;
        ast_base * base;
#define AST_TYPE(ident, identstr, structdef) struct ast_##ident * as_##ident;
        AST_NODES
#undef AST_TYPE
    };
    ast_type type;
} AST;

typedef struct {
    AST field;
    AST type; // may be NULL_AST if the type is the same as the next field
} AST_typed_field;


typedef struct {
    AST ident;
    i64 value;
} AST_enum_variant;

da_typedef(AST);
da_typedef(AST_enum_variant);
da_typedef(AST_typed_field);

// generate AST node typedefs
#define AST_TYPE(ident, identstr, structdef) typedef struct ast_##ident structdef ast_##ident;
    AST_NODES
#undef AST_TYPE

#define NULL_AST ((AST){0})
#define is_null_AST(node) ((node).type == 0 || (node).rawptr == NULL)

extern char* ast_type_str[];
extern const size_t ast_type_size[];

AST new_ast_node(arena* restrict alloca, ast_type type);
void dump_tree(AST node, int n);