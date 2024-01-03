#pragma once
#define PHOBOS_H

#include "../orbit.h"
#include "../dynarr.h"

#include "lexer.h"

dynarr_lib_h(lexer_state);

typedef struct program_tree_s {
} program_tree;

program_tree* phobos_perform_frontend();