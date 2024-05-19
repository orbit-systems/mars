#include "asmprinter.h"

void debugAsmPrinter(AsmModule* am) {
    //printf("func_len: %d\n", am->functions_len);
    foreach_non_da(AsmFunction* curr_func, am->functions, am->functions_len, [count]) {
        printf("Function: " str_fmt "\n", str_arg(curr_func->sym->name));
        foreach_non_da(AsmBlock* curr_block, curr_func->blocks, curr_func->num_blocks, [count]) {
            printf(str_fmt ":\n", str_arg(curr_block->label));
            foreach(AsmInst *curr_inst, *curr_block) {
                printf(str_fmt"\n", str_arg(curr_inst->template->asm_string));
            }
        }
    }
}
