/* This is a template pass to use when creating new passes. The passes need to be registered in pass.c and passes.h, in the factory function.
 */

#include "deimos.h"

IR_Module* ir_pass_template(IR_Module* mod) {
	return mod;
}