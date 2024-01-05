#pragma once
#define PHOBOS_H

#include "../orbit.h"
#include "../dynarr.h"

#include "lexer.h"

dynarr_lib_h(lexer_state);

typedef struct {
    
} compilation_unit;

compilation_unit* phobos_perform_frontend();