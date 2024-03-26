#include "orbit.h"
#include "deimos_dag.h"
#include "../phobos/ast.h"

//this pass legalises the phobos AST into a deimos DAG

#define DAG_ARENA_SIZE 0x100000

void recurse_legalisation_ast(DAG parent_link, DAG parent, AST node, int depth);

arena dag_alloca;

DAG pass_legalise(AST base_node) {
	//setup new DAG
	dag_alloca = arena_make(DAG_ARENA_SIZE);

	DAG new_dag = new_dag_node(&dag_alloca, dagtype_entry);

	recurse_legalisation_ast(new_dag.as_entry->node, new_dag, base_node, 0);
}

void recurse_legalisation_ast(DAG parent_link, DAG parent, AST node, int depth) {
	switch (node.type) {
		case astype_unary_op_expr: {
			parent_link = new_dag_node(&dag_alloca, dagtype_unary_op);
			parent_link.as_unary_op->operator = node.as_unary_op_expr->op; //may need to clone
			recurse_legalisation_ast(parent_link.as_unary_op->operand, parent_link, node.as_unary_op_expr->inside, depth+1);
			break;
		}

		default: {
			if (is_null_AST(node)) {
				general_error("recurse_legalisation_ast Depth %d: Encountered NULL AST node.\n", depth);
				return;
			}
			if (node.type > astype_COUNT) {
				general_error("recurse_legalisation_ast Depth %d: Encountered OOB AST node %d", depth, node.type);
				return;
			}
			general_error("recurse_legalisation_ast Depth %d: Encountered unhandled AST node: %s", depth, ast_type_str[node.type]);
			return;
		}
	}	
}