#include "deimos.h"

/* pass "stackprom" - promote stackallocs to registers

*/

da_typedef(IR_PTR);

da(IR_PTR) alloca_list = {0};

static void stackprom_f(IR_Function* f) {
    da_clear(&alloca_list);

    // add stackallocs to the list
    FOR_URANGE(b, 0, f->blocks.len) {
        FOR_URANGE(i, 0, f->blocks.at[b]->len) {
            IR* inst = f->blocks.at[b]->at[i];
            if (inst->tag == IR_STACKALLOC) {
                da_append(&alloca_list, inst);
            }
        }
    }

    TODO("");
}

IR_Module* ir_pass_stackprom(IR_Module* mod) {
    if (alloca_list.at == NULL) {
        da_init(&alloca_list, 4);
    }

    FOR_URANGE(i, 0, mod->functions_len) {
        stackprom_f(mod->functions[i]);
    }

    return mod;
}