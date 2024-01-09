#pragma once
#define PHOBOS_H

#include "orbit.h"
#include "dynarr.h"

#include "lexer.h"
#include "parser.h"

dynarr_lib_h(lexer)
dynarr_lib_h(parser)

typedef struct {
    string module_name;
} compilation_unit;

compilation_unit* phobos_perform_frontend();

typedef struct {

} parser_file;