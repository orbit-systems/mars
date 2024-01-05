#pragma once
#define PHOBOS_PARSER_H

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
    AST_TYPE(expr_paren, "parenthesis", { \
        ast_base base; \
        AST sub; \
    }) \

// generate the enum tags for the AST tagged union
typedef u16 ast_type; enum {
    astype_invalid,
#   define AST_TYPE(ident, identstr, structdef) astype_##ident,
        AST_NODES
#   undef AST_TYPE
    astype_COUNT,
};

// generate tagged union AST type
typedef struct {
    ast_type type;
    union {
        ast_base * base;
#       define AST_TYPE(ident, identstr, structdef) struct ast_##ident * ident;
            AST_NODES
#       undef AST_TYPE
    };
} AST;

// generate AST node typedefs
#define AST_TYPE(ident, identstr, structdef) typedef struct ast_##ident structdef ast_##ident;
    AST_NODES
#undef AST_TYPE

#undef AST_NODES