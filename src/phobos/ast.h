#pragma once
#define PHOBOS_AST_H

#include "orbit.h"
#include "lex.h"
#include "arena.h"
#include "exactval.h"
#include "type.h"

typedef struct {
    Token* start;
    Token* end;
    // type* T; /* tentatively deleting this, it may be useful in the future but not now */
} AstBase;

// define all the Ast node macros
#define AST_NODES \
    AST_TYPE(IdentExpr, IDENT_EXPR, { \
        union { \
            AstBase base; \
            Token* tok; \
        }; \
        struct Entity* entity; /* ease of checking and generation later */ \
    }) \
    AST_TYPE(DiscardExpr, DISCARD_EXPR, { \
        AstBase base; \
    }) \
    AST_TYPE(LitExpr, LIT_EXPR, { \
        AstBase base; \
        ExactValue value; \
    }) \
    /* generated by semantic analyzer during const eval */ \
    AST_TYPE(ExactValueExpr, EXACT_VALUE_EXPR, { \
        AstBase base; \
        ExactValue value; \
    }) \
    AST_TYPE(CompoundLitExpr, COMPOUND_LIT_EXPR, { \
        AstBase base; \
        Ast type; \
        da(Ast) elems; \
    }) \
    AST_TYPE(FunctionLitExpr, FUNCTION_LIT_EXPR, { \
        AstBase base; \
        Ast type; \
        Ast code_block; \
        \
        /* this is filled out by the checker */ \
        struct Entity** params;\
        struct Entity** returns;\
        u16 paramlen;\
        u16 returnlen;\
    }) \
    AST_TYPE(ParenExpr, PAREN_EXPR, { \
        AstBase base; \
        Ast subexpr; \
    }) \
    AST_TYPE(CastExpr, CAST_EXPR, { \
        AstBase base; \
        Ast type; \
        Ast rhs; \
        bool is_bitcast : 1; \
    }) \
    AST_TYPE(UnaryOpExpr, UNARY_OP_EXPR, { \
        AstBase base; \
        Token* op; \
        Ast inside; \
    }) \
    AST_TYPE(BinaryOpExpr, BINARY_OP_EXPR, { \
        AstBase base; \
        Token* op; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(EntitySelectorExpr, ENTITY_SELECTOR_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(SelectorExpr, SELECTOR_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
        u32 field_index; /* filled out by checker */ \
    }) \
    AST_TYPE(ReturnSelectorExpr, RETURN_SELECTOR_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
    }) \
    AST_TYPE(IndexExpr, INDEX_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast inside; \
    }) \
    AST_TYPE(SliceExpr, SLICE_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast inside_left; \
        Ast inside_right; \
    }) \
    AST_TYPE(CallExpr, CALL_EXPR, { \
        AstBase base; \
        Ast lhs; \
        da(Ast) params; \
    }) \
    AST_TYPE(AssignExpr, ASSIGN_EXPR, { \
        AstBase base; \
        union {\
            da(Ast) list; \
            Ast single; \
        } lhs;\
        Ast rhs; \
        bool single : 1; \
    }) \
    AST_TYPE(RangeExpr, RANGE_EXPR, { \
        AstBase base; \
        Ast lhs; \
        Ast rhs; \
        bool inclusive; \
    }) \
    AST_TYPE(BasicForPattern, BASIC_FOR_PATTERN, { \
        AstBase base; \
        Ast prelude; \
        Ast condition; \
        Ast iteration; \
    })\
    AST_TYPE(RangedForPattern, RANGED_FOR_PATTERN, { \
        AstBase base; \
        Ast var; \
        Ast type; \
        Ast range; \
    })\
    AST_TYPE(IdentTypePair, IDENT_TYPE_PAIR, { \
        AstBase base; \
        Ast ident; \
        Ast type; /* can be null */ \
    })\
    \
    \
    \
    \
    AST_TYPE(BlockExpr, BLOCK_EXPR, {\
        AstBase base; \
        da(Ast) stmts; \
    })\
    AST_TYPE(DeclStmt, DECL_STMT, {\
        AstBase base; \
        Ast ident; \
        Ast type; /* can be null */ \
        Ast value; /* can also be null, but not at the same time as type */ \
    })\
    AST_TYPE(ExprStmt, EXPR_STMT, {\
        AstBase base; \
        Ast expr; \
    })\
    \
    \
    \
    \
    AST_TYPE(BasicType, BASIC_TYPE, { \
        union { \
            AstBase base; \
            Token* lit; \
        }; \
    }) \
    AST_TYPE(StructType, STRUCT_TYPE, { \
            AstBase base; \
            da(Ast) fields; \
    }) \
    AST_TYPE(UnionType, UNION_TYPE, { \
            AstBase base; \
            da(Ast) fields; \
    }) \
    AST_TYPE(FnType, FN_TYPE, { \
            AstBase base; \
            da(Ast) parameters; \
            Ast block_symbol_override; /*this will be a string literal or NULL_STR if not set*/ \
            union { \
                da(Ast) returns; \
                Ast single_return; \
            }; \
            bool simple_return : 1; \
    }) \
    AST_TYPE(EnumType, ENUM_TYPE, { \
            AstBase base; \
            Ast backing_type;\
            da(Ast) variants; \
    }) \
    AST_TYPE(ArrayType, ARRAY_TYPE, { \
            AstBase base; \
            Ast subexpr; \
            Ast length; \
    }) \
    AST_TYPE(SliceType, SLICE_TYPE, { \
            AstBase base; \
            Ast subexpr; \
            bool mutable : 1; \
    }) \
    AST_TYPE(PointerType, POINTER_TYPE, { \
            AstBase base; \
            Ast subexpr; \
            bool mutable : 1; \
    }) \
    AST_TYPE(DistinctType, DISTINCT_TYPE, { \
            AstBase base; \
            Ast subexpr; \
    }) \


// generate the enum tags for the Ast tagged union
typedef u16 ast_type; enum {
    AST_INVALID,
#define AST_TYPE(ident, enumident, structdef) AST_##enumident,
    AST_NODES
#undef AST_TYPE
    AST_COUNT,
};

// generate tagged union Ast type
typedef struct Ast {
    union {
        void* rawptr;
        AstBase * base;
#define AST_TYPE(ident, enumident, structdef) struct Ast##ident * ident;
        AST_NODES
#undef AST_TYPE
    };
    ast_type type;
} Ast;

da_typedef(Ast);

// generate Ast node typedefs
#define AST_TYPE(ident, enumident, structdef) typedef struct Ast##ident structdef Ast##ident;
    AST_NODES
#undef AST_TYPE

#define NULL_AST ((Ast){0})
#define is_null_AST(node) ((node).type == 0 || (node).rawptr == NULL)

extern char* ast_type_str[];
extern const size_t ast_type_size[];

Ast new_ast_node(Arena* alloca, ast_type type);