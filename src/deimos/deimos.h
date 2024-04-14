#pragma once
#define DEIMOS_H

#include "../phobos/ast.h"
#include "pass.h"
#include "deimos_dag.h"
#include "dag_utils.h"

void deimos_init(AST base_node);