#include "deimos.h"

void deimos_init(AST base_node) {
	init_passes();
	run_passes(base_node);
}