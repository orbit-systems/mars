#pragma once
#define PHOBOS_H

#include "orbit.h"
#include "dynarr.h"

#include "lexer.h"
#include "parser.h"

dynarr_lib_h(lexer)
dynarr_lib_h(parser)

typedef struct {
    parser p;
    string module_name;
} mars_file;

dynarr_lib_h(mars_file)

typedef struct {
    string module_name;
    dynarr(mars_file) files;
} compilation_unit;

compilation_unit* phobos_perform_frontend();