#include "deimos.h"

void process_ast(AST base_node) {
	run_passes(base_node);
}