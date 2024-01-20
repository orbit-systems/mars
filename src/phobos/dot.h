#pragma once
#define PHOBOS_DOT_H

#include "orbit.h"
#include "ast.h"

void emit_dot(string path, AST node);
void recurse_dot(AST node, fs_file* file, int n, int uID);