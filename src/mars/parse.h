#ifndef MARS_PARSE_H
#define MARS_PARSE_H

#include "lex.h"

typedef struct {i32 _;} Index;

typedef enum: u8 {
    AST_INVALID = 0,

    AST_IDENT,
    AST_CONST_NULL,
    AST_CONST_FALSE,
    AST_CONST_TRUE,

    AST_TY_ISIZE,
    AST_TY_USIZE,
    AST_TY_INT,
    AST_TY_UINT,
    AST_TY_FLOAT,
    AST_TY_TYPE,
    AST_TY_VOID,
    AST_TY_NORETURN,
    AST_TY_SELF,
} AstNodeKind;

#endif // MARS_PARSE_H
