#include "deimos.h"

arena deimos_alloca;

void deimos_init(AST base_node) {
	deimos_alloca = arena_make(DEIMOS_ARENA_SIZE);

	// (sandwichman): hijacking this to test

	// init_passes();
	// run_passes(base_node);
}