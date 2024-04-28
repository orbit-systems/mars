#include "deimos.h"
#include "passes/passes.h"
/* (sandwich): okay, heres how this is going to work.

the function generate_ir() encompasses the simple AST -> IR
translation phase. it does not do any optimizations.
this is explicitly NOT a structured pass in the pass system
because it is of a completely different form to the rest of 
the passes, which are IR -> IR and modify the IR directly. 
also, there is only one AST -> IR pass, so treating it as a 
general pass only makes things more complex.

the compiler driver can then schedule one or more IR passes. 
They can be optimization passes, analysis passes, but their 
common trait is that they operate on the IR structure itself.

*/

void deimos_run(mars_module* main_mod) {
    IR_Module* ir_mod = ir_generate(main_mod);

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
    FOR_URANGE(i, 1, len - 1) {
        str[i] = 'A' + rand() % ('Z' - 'A'); //generates random ascii from 0x21 (!) to 0x7E (~)
    }
    str[len] = '\0';
    return str;
}