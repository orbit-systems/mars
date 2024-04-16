#include "orbit.h"
#include "../deimos_ir.h"
#include "../../phobos/ast.h"

//this pass converts the phobos AST into a deimos IR

void ast_to_ir_walk(AST node, int depth);

da(IR) pass_ast_to_ir(AST base_node) {
	ast_to_ir_walk(base_node, 0);

	//return (IR){};
}

void ast_to_ir_walk(AST node, int depth) {
	switch (node.type) {
		//statements

		default: {
			if (is_null_AST(node)) {
				general_warning("FIXME: ast_to_ir_walk Depth %d: Leaked a NULL AST node.\n", depth);
				break;
			}
			if (node.type > AST_COUNT) {
				general_warning("SEVERE FIXME: ast_to_ir_walk Depth %d: Encountered OOB AST node %d\n", depth, node.type);
				break;
			}
			general_error("FIXME: ast_to_ir_walk Depth %d: Encountered unhandled AST node: %s", depth, ast_type_str[node.type]);
			break;
		}
	}	
	//printf("at depth: %d\n", depth);
}