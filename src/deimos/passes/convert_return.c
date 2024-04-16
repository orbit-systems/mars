#include "../deimos.h"
#include "../deimos_util.h"
#include "../../phobos/ast.h"
#include "../../phobos/entity.h"
#include "../../phobos/lex.h"

#define PCR_RETURN_EARLY(base_node, parent, depth) if (pcr_walk_ast((base_node), (parent), (depth + 1))) return 1;

AST pass_convert_return(AST base_node) {
	pcr_walk_ast(base_node, NULL_AST, 0);
}

int pcr_walk_ast(AST node, AST parent, int depth) {
	switch(node.type) {
		case AST_decl_stmt: {
			//ignore lhs
			PCR_RETURN_EARLY(node.as_decl_stmt->rhs, node, depth);
			break;
		}
		case AST_func_literal_expr: {
			//ignore type
			PCR_RETURN_EARLY(node.as_func_literal_expr->code_block, node, depth);
			break;
		}

		case AST_block_stmt: {
			FOR_URANGE(i, 0, node.as_block_stmt->stmts.len) {
				if (node.as_block_stmt->stmts.at[i].type == AST_return_stmt) {
					AST return_stmt = node.as_block_stmt->stmts.at[i];
					FOR_URANGE(j, 0, return_stmt.as_return_stmt->returns.len) {
						AST potential_non_identifier = return_stmt.as_return_stmt->returns.at[j];
						if (potential_non_identifier.type != AST_identifier_expr) {
							//this is STUPIDLY bad practice, but new ast nodes are going to be
							//part of the deimos alloca arena
							AST new_decl_stmt = new_ast_node(&deimos_alloca, AST_decl_stmt);
							general_warning("FIXME: pcr_walk_ast is assuming the typing info for the statement");
							general_warning("FIXME: \"return a + b\", where typeof(a) == typeof(b) == i64");
							general_warning("FIXME: this is BAD. fix it.");
							general_warning("FIXME: it is currently extracting the et from the a identifier.");
							general_warning("FIXME: this pass also only works on returns of ONE value currently.");
							//we cant yet fill out the identifier, but we CAN copy over the decl_stmt info
							new_decl_stmt.as_decl_stmt->rhs = potential_non_identifier;

							AST new_identifier = new_ast_node(&deimos_alloca, AST_identifier_expr);
							//potential CAE issue? new_entity() extracts info from
							//the AST, but the new AST nodes arent yet built. hopefully no issues here
							//extract entity table
							//entity* identifier_a_et = potential_non_identifier.as_binary_op_expr->lhs.as_identifier_expr->entity;

							general_warning("FIXME: just when you think it couldnt get worse: we bodge new_entity with NULL et");
							general_warning("FIXME: so that new_entity respects us enough to give us a valid entity*.");
							general_warning("FIXME: this is because we arent yet running the checker, which WOULD have populated");
							general_warning("FIXME: the entity* inside of all identifier_expr.");

							string identifier_name = to_string(random_string(8));

							new_identifier.as_identifier_expr->entity = new_entity(NULL, identifier_name, new_decl_stmt);
							printf("created new identifier with identifier name %s\n", clone_to_cstring(identifier_name));

							//create token for identifier
							token* new_token = malloc(sizeof(*new_token));
							if (new_token == NULL) general_error("Failed to allocate space for new identifier token.");
							new_token->type = TOK_IDENTIFIER;
							new_token->text = identifier_name;

							//if (identifier_a_et == NULL) general_error("FUCK %s", ast_type_str[potential_non_identifier.type]);
							//new_identifier.as_identifier_expr->entity = new_entity(identifier_a_et, to_string(random_string(8)), new_decl_stmt);
						}
					}
				}
			}
			break;
		}

		default:
			general_error("pcr_walk_ast: did not parse node: %s at depth %d", ast_type_str[node.type], depth);
			break;
	}
}