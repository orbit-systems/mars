#include <stdalign.h>

#include "../orbit.h"
#include "lexer.h"
#include "ast.h"

size_t ast_type_size[] = {
    0,
#define AST_TYPE(ident, identstr, structdef) sizeof(ast_##ident),
    AST_NODES
#undef AST_TYPE
    0
};

char* ast_type_str[] = {
    "invalid",
#define AST_TYPE(ident, identstr, structdef) identstr,
    AST_NODES
#undef AST_TYPE
    "COUNT",
};