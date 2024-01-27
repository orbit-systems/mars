#pragma once
#define PHOBOS_H

#include "orbit.h"
#include "da.h"

#include "lexer.h"
#include "parser.h"
#include "ast.h"

da_typedef(lexer);
da_typedef(parser);

typedef struct {
    string path;
    string src;
} mars_file;

da_typedef(mars_file);

typedef struct {
    struct mars_module ** at;
    size_t len;
    size_t cap;
} mmodule_list;

typedef struct {
    string module_name;
    da(mars_file) files;
    mmodule_list import_list;

    da(AST) program_tree;
    arena AST_alloca;

} mars_module;

mars_module* parse_target_module(string input_path);

// creates a compilation unit from a list of parsers.
// stitches the unchecked ASTs together and such
mars_module* create_module(da(parser)* pl, arena alloca);

// find the source file given a snippet of code
// NOTE: the snippet must be an actual substring (is_within() must return true) of one of the files
mars_file* find_source_file(mars_module* cu, string snippet);