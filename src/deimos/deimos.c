#include "deimos.h"
#include "passes/passes.h"

void deimos_run(mars_module* main_mod, IR_Module* passthrough) {
	IR_Module* ir_mod;
	if (!mars_flags.use_lltd) {
 		ir_mod = ir_generate(main_mod);
	} else {
		ir_mod = passthrough;
	}

    register_passes();
    run_passes(ir_mod);
}

char* random_string(int len) {
    if (len < 3) {
        general_error("random_string() needs to be called with len >= 3");
    }
    char* str = malloc(len + 1);
    if (str == NULL) general_error("Failed to allocate %d bytes for random_string", len + 1);
    str[0] = '#';
    str[len - 1] = '#';
    for_urange(i, 1, len - 1) {
        str[i] = 'A' + rand() % ('Z' - 'A'); //generates random ascii from 0x21 (!) to 0x7E (~)
    }
    str[len] = '\0';
    return str;
}