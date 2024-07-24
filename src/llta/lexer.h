#pragma once
#define LEXER_H
#include "iron/iron.h"
#include "mars/term.h"

FeModule* llta_parse_ir(string path);

da_typedef(string);

typedef struct {
    string tok;
    int line;
} icarus_token;

da_typedef(icarus_token);