#include "asmprinter.h"

void debugAsmPrinter(AsmModule* am) {
    for_urange(i, 0, am->functions_len) {
        AsmFunction* curr_func = am->functions[i];
        printf("Function: %s\n", curr_func->sym->name.raw);
        for_urange(j, 0, curr_func->num_blocks) {
            AsmBlock* curr_block = curr_func->blocks[i];
            printf("Label: %s\n", curr_block->label.raw);
            for_urange(k, 0, curr_block->len) {
                AsmInst* curr_inst = curr_block->at[k];
                printf("%s", curr_inst->template->asm_string.raw);
            }
        }
    }
}
