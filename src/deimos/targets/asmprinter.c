#include "asmprinter.h"

void debug_asm_printer(AsmModule* am) {
    //printf("func_len: %d\n", am->functions_len);
    foreach_non_da(AsmFunction* curr_func, am->functions, am->functions_len, [count]) {
        printf("function: " str_fmt "\n", str_arg(curr_func->sym->name));
        foreach_non_da(AsmBlock* curr_block, curr_func->blocks, curr_func->num_blocks, [count]) {
            printf("  "str_fmt ":\n", str_arg(curr_block->label));
            foreach(AsmInst *curr_inst, *curr_block) {
                printf("    "str_fmt"\n", str_arg(curr_inst->template->asm_string));
            }
        }
    }
}

void print_asm_inst(AsmModule* m, AsmInst* inst);

void asm_printer(AsmModule* m) {

    for_urange(i, 0, m->functions_len) {
        AsmFunction* f = m->functions[i];
        
        printf("" str_fmt ":\n", str_arg(f->sym->name));
        for_urange(j, 0, f->num_blocks) {
            AsmBlock* b = f->blocks[i];

            printf("  ." str_fmt ":\n", str_arg(b->label));
            for_urange(k, 0, b->len) {

                AsmInst* inst = b->at[k];

                printf("    ");
                print_asm_inst(m, inst);
                printf("\n");
            }
        }
    }
}

#define skip_whitespace(c, i) while (c == ' ') c = fmt.raw[i++];

static size_t scan_uint(char* str, size_t* index) {
    size_t val = 0;
    while ('0' <= str[*index] && str[*index] <= '9') {
        val = val * 10 + (str[*index] - '0');
        *index++;
    }
    return val;
}

void print_asm_inst(AsmModule* m, AsmInst* inst) {

    TargetInstInfo* info = inst->template;
    string fmt = info->asm_string;

    for (size_t i = 0; i < fmt.len; i++) {
        char c = fmt.raw[i];

        if (c == '{') {
            // format start!!

            i++; // increment past the {

            // skip until not whitespace
            skip_whitespace(c, i);

            if (strncmp(&fmt.raw[i], "in", 2) == 0) {
                // we have an in!


                TODO("in printing");

            } else if (strncmp(&fmt.raw[i], "out", 3) == 0) {
                // we have an out!

                i += 3; // skip past the 'out'
                skip_whitespace(c, i);
                size_t out_index = scan_uint(fmt.raw, &i);

                VReg* out = inst->outs[i];
                
                if (out->real == REAL_REG_UNASSIGNED) {
                    // print a virtual register

                    printf("v%p", out); // unreadable as fuck
                } else {
                    // print a real register

                    TargetRegisterInfo* real = &m->target->regclasses[out->required_regclass].regs[out->real];

                    printf(str_fmt, out->real, str_arg(real->name));
                }

                while (c != '}') c = fmt.raw[i++]; // skip until }
                continue;

            } else if (strncmp(&fmt.raw[i], "imm", 3) == 0) {
                // we have an imm!
                TODO("immedaite printing");
            } else {
                CRASH("invalid assembly format");
            }
        } else {
            printf("%c", c);
        }
    }

}