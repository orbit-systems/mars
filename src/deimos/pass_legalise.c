#include "orbit.h"
#include "deimos_ir.h"
#include "../phobos/ast.h"

//this pass legalises the phobos AST into a deimos DAG

#define IR_ARENA_SIZE 0x100000

void recurse_legalisation_ast(AST node, int depth);

arena ir_alloca;

IR pass_legalise(AST base_node) {
	//setup new DAG
	ir_alloca = arena_make(IR_ARENA_SIZE);

	//need to do some fucky shit.
	recurse_legalisation_ast(base_node, 0);

	return (IR){};
}

void recurse_legalisation_ast(AST node, int depth) {
	switch (node.type) {
		//statements

		default: {
			if (is_null_AST(node)) {
				general_warning("FIXME: recurse_legalisation_ast Depth %d: Leaked a NULL AST node.\n", depth);
				break;
			}
			if (node.type > AST_COUNT) {
				general_warning("SEVERE FIXME: recurse_legalisation_ast Depth %d: Encountered OOB AST node %d\n", depth, node.type);
				break;
			}
			general_warning("FIXME: recurse_legalisation_ast Depth %d: Encountered unhandled AST node: %s", depth, ast_type_str[node.type]);
			break;
		}
	}	
	//printf("at depth: %d\n", depth);
}