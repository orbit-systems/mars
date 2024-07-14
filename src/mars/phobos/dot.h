#pragma once
#define PHOBOS_DOT_H

#include "common/orbit.h"
#include "ast.h"

void emit_dot(string path, da(AST) nodes);
void recurse_dot(AST node, fs_file* file, int n, int uID);