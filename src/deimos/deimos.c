#include "deimos.h"

arena deimos_alloca;

void generate_return(AST node, da(IR)* ir_stream);

void deimos_init(AST base_node) {
	deimos_alloca = arena_make(DEIMOS_ARENA_SIZE);

	// (sandwichman): hijacking this to test
	da(IR) ir_stream = {0};
	da_init(&ir_stream, 1);
	
	generate_return(base_node, &ir_stream);
	print_ir(ir_stream);	
	


	// init_passes();
	// run_passes(base_node);
}