#include "orbit.h"
#include "pass.h"

//implements the pass system for processing the aphelion AST and transforming it into a DAG

/*
 *KAYLA YOU SILLY GIT
 *fix the architectural problem with the dag
 *make the dag_base contain a base pointer to the dag itself or something
 *you gotta fix this, girlie
 *
 *
 *
 */

da(pass) passes;

void add_pass(pass_id id, void (*callback)(), char* pass_name) {
	pass new_pass;
	new_pass.id = id;
	new_pass.pass_name = to_string(pass_name);
	new_pass.callback = callback;
	da_append(&passes, new_pass);
}

void init_passes() {
	da_init(&passes, 1);
}

void run_passes(AST base_node) {
	//execute passes in order of init
	//the legalising pass is special, and doesnt actually take in a DAG, it takes in the AST.
	 

	//ast to ast transforms are handled "special"
	printf("AST->AST Passes:\n");
	printf("Running pass 0: Convert return_stmt !-> identifier_expr to return_stmt -> identifier by inserting dummy identifiers.\n");
	AST current_ast = pass_convert_return(base_node);
	printf("Running pass 1: AST to IR transform pass | Crude transform from AST to IR\n");
	da(IR) current_ir = pass_ast_to_ir(base_node);

	FOR_URANGE(i, 0, passes.len) {
		pass current_pass = passes.at[i];

		char* pass_name = clone_to_cstring(current_pass.pass_name); //FIXME: is this hacky?
		printf("Running pass %d: %s\n", i + 1, pass_name);
		free(pass_name);

		switch(current_pass.id) {
			case PASS_IR_2_IR:
				current_pass.IR_2_IR(current_ir);
				break;
			default:
				general_error("Unexpected pass_id of %d", current_pass.id);
				break;
		}
	}
}