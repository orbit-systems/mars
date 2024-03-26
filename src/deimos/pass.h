#pragma once
#define DEIMOS_PASS_H

#include "../phobos/ast.h"
#include "deimos_dag.h"

void run_passes(AST base_node);

DAG pass_legalise(AST base_node);