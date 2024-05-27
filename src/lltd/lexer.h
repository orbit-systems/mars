#pragma once
#define LEXER_H
#include "deimos/ir.h"
#include "term.h"

AIR_Module* lltd_parse_ir(string path);

da_typedef(string);

typedef struct {
    string tok;
    int line;
} icarus_token;

da_typedef(icarus_token);