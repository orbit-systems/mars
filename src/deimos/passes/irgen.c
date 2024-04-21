#include "deimos.h"
#include "passes.h"

IR_Module* ir_pass_generate(mars_module* mod) {
	IR_Module* mod = ir_new_module(mod->module_name);
	
	/* do some codegen shit prolly */

	return mod;
}