#include "deimos.h"
#include "passes/passes.h"

da(Pass) deimos_passes;

void add_pass(char* name, void (*callback)(), pass_type type) {
	if (deimos_passes.at == NULL) da_init(&deimos_passes, 1);
	da_append(&deimos_passes, ((Pass){.name = name, .callback = callback, .type = type}));
}

void register_passes() {
	add_pass("general cleanup after ir generation", ir_pass_org, PASS_IR_TO_IR);
}

void run_passes(IR_Module* current_program) {
	ir_print_module(current_program);
	FOR_URANGE(i, 0, deimos_passes.len) {
		printf("Running pass: %s\n", deimos_passes.at[i].name);
		if (deimos_passes.at[i].type == PASS_IR_TO_IR) {
			current_program = deimos_passes.at[i].ir_callback(current_program);
		} else {
			general_error("Unexpected pass type!");
		}
		ir_print_module(current_program);
	}
}