#pragma once
#define PHOBOS_CHECKER_H

#include "phobos.h"
#include "parser.h"
#include "entity.h"
#include "type.h"
#include "term.h"

// emit an error that highlights an AST node
#define error_at_node(mod_ptr, node, msg, ...) do { \
    string ast_str = str_from_tokens(*((node).base->start), *((node).base->end));                     \
    mars_file* source_file = find_source_file(mod_ptr, ast_str);                                      \
    error_at_string(source_file->path, source_file->src, ast_str, message __VA_OPT__(,) __VA_ARGS__): \
} while (0)
