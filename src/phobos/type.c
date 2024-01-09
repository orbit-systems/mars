#include "orbit.h"
#include "lexer.h"
#include "error.h"
#include "type.h"
#include "arena.h"

size_t mt_type_size[] = {
    0,
#define TYPE(ident, identstr, structdef) sizeof(mtype_##ident),
    TYPE_NODES
#undef TYPE
    0
};

char* mt_type_str[] = {
    "invalid",
#define TYPE(ident, identstr, structdef) identstr,
    TYPE_NODES
#undef TYPE
    "COUNT",
};