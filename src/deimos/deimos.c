#include "deimos.h"

void deimos_init(AST base_node) {
	run_passes(base_node);
}