#pragma once
#define LEXER_H
#include "atlas.h"
#include "term.h"

AtlasModule* llta_parse_ir(string path);

da_typedef(string);

typedef struct {
    string tok;
    int line;
} icarus_token;

da_typedef(icarus_token);