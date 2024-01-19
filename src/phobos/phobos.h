#pragma once
#define PHOBOS_H

#include "orbit.h"
#include "da.h"

#include "lexer.h"
#include "parser.h"

da_typedef(lexer);
da_typedef(parser);

typedef struct {
    lexer l;
    parser p;
} mars_file;

da_typedef(mars_file);

typedef struct {
    string module_name;
    da(mars_file) files;
} compilation_unit;

compilation_unit* phobos_perform_frontend();