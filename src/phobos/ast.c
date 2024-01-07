#include "../orbit.h"
#include "lexer.h"
#include "ast.h"

char* ast_type_str[] = {
    "invalid",
#define AST_TYPE(ident, identstr, structdef) identstr,
    AST_NODES
#undef AST_TYPE
    "COUNT",
};