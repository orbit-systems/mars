#pragma once
#define PHOBOS_DOT_H

void emit_dot(string path, AST node);
void recurse_dot(AST node, fs_file* file, int n, int uID);