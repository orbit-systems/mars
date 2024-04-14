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

	DAG new_dag = new_dag_node(&dag_alloca, dagtype_entry, 0);

	recurse_legalisation_ast(new_dag.as_entry->node, new_dag, base_node, 0);

	return new_dag;
}

void recurse_legalisation_ast(DAG parent_link, DAG parent, AST node, int depth) {
	switch (node.type) {
		//statements
		case AST_decl_stmt: { //?FIXME: remove the dag2tdag pass and do assign here
			parent_link = new_dag_node(&dag_alloca, dagtype_decl_stmt, depth);
			da_init(&parent_link.as_decl_stmt->lhs, node.as_decl_stmt->lhs.len);
			FOR_URANGE(i, 0, node.as_decl_stmt->lhs.len) {
				//push all identifier nodes into a dynamic array
				AST identifier = node.as_decl_stmt->lhs.at[i];
				if (identifier.type != AST_identifier_expr) general_error("recurse_legalisation_ast Depth: %d: Found unexpected non-identifier in LHS of declaration statement\n", depth);
				da_append(&parent_link.as_decl_stmt->lhs, ((dag_identifier_entity){.identifier = identifier.as_identifier_expr->tok, 
																	   			   .entity = identifier.as_identifier_expr->entity,
																				   .is_volatile = node.as_decl_stmt->is_volatile,
																				   .is_uninit   = node.as_decl_stmt->is_uninit}));

			}
			general_warning("FIXME: AST_decl_stmt parsing is leaking identifiers in RHS.");
			recurse_legalisation_ast(parent_link.as_decl_stmt->rhs, parent_link, node.as_decl_stmt->rhs, depth + 1);
			break;
		}
		case AST_block_stmt: {
			parent_link = new_dag_node(&dag_alloca, dagtype_block_stmt, depth);
			da_init(&parent_link.as_block_stmt->statements, node.as_block_stmt->stmts.len);
			FOR_URANGE(i, 0, node.as_block_stmt->stmts.len) {
				recurse_legalisation_ast(parent_link.as_block_stmt->statements.at[i], parent_link, node.as_block_stmt->stmts.at[i], depth + 1);
				parent_link.as_block_stmt->statements.len += 1; //FIXME: this is hacky.
			} 
			general_warning("recurse_legalisation_ast FIXME: we're still hacking the DA system.");
			break;
		}
		case AST_return_stmt: {
			parent_link = new_dag_node(&dag_alloca, dagtype_return_stmt, depth);
			da_init(&parent_link.as_return_stmt->returns, node.as_return_stmt->returns.len);
			general_warning("FIXME: AST_return_stmt parsing is leaking identifiers in returns.");
			FOR_URANGE(i, 0, node.as_return_stmt->returns.len) {
				recurse_legalisation_ast(parent_link.as_return_stmt->returns.at[i], parent_link, node.as_return_stmt->returns.at[i], depth + 1);
				parent_link.as_return_stmt->returns.len += 1; //FIXME: this is hacky.
			}
			general_warning("recurse_legalisation_ast FIXME: we're still hacking the DA system.");
			break;
		}
		//literals
		case AST_func_literal_expr: {
			parent_link = new_dag_node(&dag_alloca, dagtype_func_literal, depth);
			recurse_legalisation_ast(parent_link.as_func_literal->code_block, parent_link, node.as_func_literal_expr->code_block, depth + 1);
			break;
		}
		//operators
		case AST_binary_op_expr: {
			parent_link = new_dag_node(&dag_alloca, dagtype_binary_op, depth);
			parent_link.as_binary_op->op = node.as_binary_op_expr->op; //?FIXME: clone?
			//in a binary op, if its a literal or an identifier, dont parse. otherwise, parse.
			//keep the AST links, but dont parse.
			if (node.as_binary_op_expr->lhs.type != AST_literal_expr && node.as_binary_op_expr->lhs.type != AST_identifier_expr) {
				recurse_legalisation_ast(parent_link.as_binary_op->Dlhs, parent_link, node.as_binary_op_expr->lhs, depth + 1);
				parent_link.as_binary_op->lhsAST = false;
			} else {
				parent_link.as_binary_op->Alhs = node.as_binary_op_expr->lhs;
				parent_link.as_binary_op->lhsAST = true;
			} 

			if (node.as_binary_op_expr->rhs.type != AST_literal_expr && node.as_binary_op_expr->rhs.type != AST_identifier_expr) {
				recurse_legalisation_ast(parent_link.as_binary_op->Drhs, parent_link, node.as_binary_op_expr->rhs, depth + 1);
				parent_link.as_binary_op->rhsAST = false;
			} else {
				parent_link.as_binary_op->Arhs = node.as_binary_op_expr->rhs;
				parent_link.as_binary_op->rhsAST = true;
			}
			
			break;
		}	
		case AST_unary_op_expr: {
			parent_link = new_dag_node(&dag_alloca, dagtype_unary_op, depth);
			parent_link.as_unary_op->operator = node.as_unary_op_expr->op; //?FIXME: may need to clone
			recurse_legalisation_ast(parent_link.as_unary_op->operand, parent_link, node.as_unary_op_expr->inside, depth + 1);
			break;
		}

		default: {
			if (is_null_AST(node)) {
				general_warning("FIXME: recurse_legalisation_ast Depth %d: Leaked a NULL AST node.\n", depth);
				break;
			}
			if (node.type > AST_COUNT) {
				general_warning("SEVERE FIXME: recurse_legalisation_ast Depth %d: Encountered OOB AST node %d from parent %s\n", depth, node.type, dag_type_str[parent.type]);
				break;
			}
			general_warning("FIXME: recurse_legalisation_ast Depth %d: Encountered unhandled AST node: %s from parent %s", depth, ast_type_str[node.type], dag_type_str[parent.type]);
			break;
		}
	}	
	//printf("at depth: %d\n", depth);
}