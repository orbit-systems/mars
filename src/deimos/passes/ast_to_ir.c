#include "../deimos.h"
#include "../deimos_ir.h"
#include "../../phobos/ast.h"

//this pass converts the phobos AST into a deimos IR

void ast_to_ir_walk(AST node, AST parent, int depth, da(IR) *ir_elems);

da(IR) pass_ast_to_ir(AST base_node) {
	da(IR) ir_elems;
	da_init(&ir_elems, 1);
	ast_to_ir_walk(base_node, NULL_AST, 0, &ir_elems);

	//printf()

	print_ir(ir_elems);

	//return (IR){};
}

//the AST walker works by going to the leaf nodes, emitting IR, then crawling back up and emitting IR again, until the entire
//ast has been converted.

void ast_to_ir_walk(AST node, AST parent, int depth, da(IR) *ir_elems) {
	switch (node.type) {
		//statements
		case AST_decl_stmt: {
			switch (node.as_decl_stmt->rhs.type) {
				case AST_func_literal_expr:
					IR func_literal_ir = new_ir_entry(&deimos_alloca, ir_type_function_label);
					func_literal_ir.as_function_label->label_text = node.as_decl_stmt->lhs.at[0].as_identifier_expr->tok->text;
					da_append(ir_elems, func_literal_ir);	

					//emit args
					AST fn_type = node.as_decl_stmt->rhs.as_func_literal_expr->type;
					FOR_URANGE(i, 0, fn_type.as_fn_type_expr->parameters.len) {
						IR argument_mov = new_ir_entry(&deimos_alloca, ir_type_arg_mov);
						argument_mov.as_arg_mov->identifier_name = fn_type.as_fn_type_expr
																	     ->parameters.at[i].field.as_identifier_expr
																	     ->tok->text;
						argument_mov.as_arg_mov->argument = i;
						da_append(ir_elems, argument_mov);
					}
					ast_to_ir_walk(node.as_decl_stmt->rhs.as_func_literal_expr->code_block, node, depth + 1, ir_elems);				
					break;
				case AST_binary_op_expr:
					ast_to_ir_walk(node.as_decl_stmt->rhs, node, depth + 1, ir_elems);
					break;
				default:
					general_error("Unhandled AST_decl_stmt lowering for node %s", ast_type_str[node.as_decl_stmt->rhs.type]);
			}
			break;
		}
		case AST_block_stmt: {
			FOR_URANGE(i, 0, node.as_block_stmt->stmts.len) {
				ast_to_ir_walk(node.as_block_stmt->stmts.at[i], node, depth + 1, ir_elems);
			}
			break;
		}

		case AST_binary_op_expr: {
			general_warning("FIXME: ast_to_ir is assuming type(lhs) == type(rhs) == identifier_expr");
			IR binary_op = new_ir_entry(&deimos_alloca, ir_type_binary_op);
			binary_op.as_binary_op->lhs = node.as_binary_op_expr->lhs.as_identifier_expr->tok->text;
			binary_op.as_binary_op->rhs = node.as_binary_op_expr->rhs.as_identifier_expr->tok->text;
			binary_op.as_binary_op->out = parent.as_decl_stmt->lhs.at[0].as_identifier_expr->tok->text;
			operator ir_op = NO_OP;
			string ast_op = node.as_binary_op_expr->op->text;
			if(string_eq(ast_op, to_string("+"))) ir_op = ADD;
			if(string_eq(ast_op, to_string("-"))) ir_op = SUB;
			if(string_eq(ast_op, to_string("*"))) ir_op = MUL;
			if(string_eq(ast_op, to_string("/"))) ir_op = DIV;
			binary_op.as_binary_op->op = ir_op;
			
			if (ir_op == NO_OP) general_error("Unhandled AST_binary_op_expr lowering for op: %s", clone_to_cstring(ast_op));

			da_append(ir_elems, binary_op);
			break;
		}

		case AST_return_stmt: {
			general_warning("FIXME: AST_return_stmt lowering only supports 1 value returns currently.");
			IR return_stmt = new_ir_entry(&deimos_alloca, ir_type_return_stmt);
			return_stmt.as_return_stmt->identifier_name = node.as_return_stmt->returns.at[0].as_identifier_expr->tok->text;
			da_append(ir_elems, return_stmt);
			break;
		}



		default: {
			if (is_null_AST(node)) {
				general_warning("FIXME: ast_to_ir_walk Depth %d: Leaked a NULL AST node.\n", depth);
				break;
			}
			if (node.type > AST_COUNT) {
				general_warning("SEVERE FIXME: ast_to_ir_walk Depth %d: Encountered OOB AST node %d\n", depth, node.type);
				break;
			}
			general_warning("FIXME: ast_to_ir_walk Depth %d: Encountered unhandled AST node: %s", depth, ast_type_str[node.type]);
			break;
		}
	}	
	//printf("at depth: %d\n", depth);
}