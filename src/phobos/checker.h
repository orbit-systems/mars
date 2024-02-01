#pragma once
#define PHOBOS_CHECKER_H

#include "phobos.h"
#include "parser.h"
#include "entity.h"
#include "type.h"
#include "term.h"

// emit an error that highlights an AST node
#define error_at_node(module, node, msg, ...) do { \
    string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                       \
    mars_file* source_file = find_source_file((module), ast_str);                                      \
    if (source_file == NULL) CRASH("CRASH: source file not found for AST node");                        \
    error_at_string(source_file->path, source_file->src, ast_str, message __VA_OPT__(,) __VA_ARGS__);   \
} while (0)

