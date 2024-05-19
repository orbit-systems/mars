#include "asmprinter.h"
#include "ptrmap.h"

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



void print_asm_inst(AsmModule* m, AsmInst* inst, bool debug_mode);

PtrMap vreg2num = {0};
uintptr_t vreg_count = 0;

// debug mode has some extra stuff
void asm_printer(AsmModule* m, bool debug_mode) {

    if (vreg2num.vals == NULL) {
        ptrmap_init(&vreg2num, 16);
    }

    for_urange(i, 0, m->functions_len) {
        AsmFunction* f = m->functions[i];
        
        printf("" str_fmt ":\n", str_arg(f->sym->name));
        for_urange(j, 0, f->num_blocks) {
            AsmBlock* b = f->blocks[i];

            printf("  ." str_fmt ":\n", str_arg(b->label));
            for_urange(k, 0, b->len) {

                AsmInst* inst = b->at[k];

                printf("    ");
                print_asm_inst(m, inst, debug_mode);
                printf("\n");
            }
        }

        ptrmap_reset(&vreg2num);
    }

    ptrmap_destroy(&vreg2num);
}

#define skip_whitespace(c, i) while (fmt.raw[i] == ' ' && i < fmt.len) {i++; c = fmt.raw[i];}

#define skip_until(char, c, i) while(fmt.raw[i] != (char) && i < fmt.len) {c = fmt.raw[i];}

static size_t scan_uint(string str, size_t* index) {
    size_t val = 0;
    while (str.len > *index && '0' <= str.raw[*index] && str.raw[*index] <= '9') {
        val = val * 10 + (str.raw[*index] - '0');
        (*index)++;
    }
    return val;
}

void print_virtual_register(size_t vnum, bool debug_mode) {
    if (debug_mode) {
        printf(STYLE_FG_Red"v%d"STYLE_Reset, vnum);
    } else {
        printf("v%d", vnum);
    }
}

void print_asm_inst(AsmModule* m, AsmInst* inst, bool debug_mode) {

    TargetInstInfo* info = inst->template;
    string fmt = info->asm_string;

    for (size_t i = 0; i < fmt.len; i++) {
        char c = fmt.raw[i];

        if (c == '{') {
            // format start!!
            printf("[[%d: %s]]", i, fmt.raw + i);
            i++; // increment past the {

            // skip until not whitespace
            skip_whitespace(c, i);

            if (strncmp(&fmt.raw[i], "in", 2) == 0) {
                // we have an in!

                i += 2; // skip past the 'in'
                skip_whitespace(c, i);
               
                size_t in_index = scan_uint(fmt, &i);

                VReg* in = inst->ins[in_index];
                
                if (in->real == REAL_REG_UNASSIGNED) {
                    // print a virtual register

                    uintptr_t vnum = (uintptr_t) ptrmap_get(&vreg2num, in);
                    if (vnum == (uintptr_t)PTRMAP_NOT_FOUND) {
                        vnum = ++vreg_count;
                        ptrmap_put(&vreg2num, in, (void*)vnum);
                    }
                    print_virtual_register(vnum, debug_mode);

                } else {
                    // print a real register

                    TargetRegisterInfo* real = &m->target->regclasses[in->required_regclass].regs[in->real];

                    printf(str_fmt, str_arg(real->name));
                }

                skip_until('}', c, i);
                continue;

            } else if (strncmp(&fmt.raw[i], "out", 3) == 0) {
                // we have an out!

                i += 3; // skip past the 'out'
                skip_whitespace(c, i);

                size_t out_index = scan_uint(fmt, &i);

                VReg* out = inst->outs[out_index];

                if (out->real == REAL_REG_UNASSIGNED) {
                    // print a virtual register

                    uintptr_t vnum = (uintptr_t) ptrmap_get(&vreg2num, out);
                    if (vnum == (uintptr_t)PTRMAP_NOT_FOUND) {
                        vnum = ++vreg_count;
                        ptrmap_put(&vreg2num, out, (void*)vnum);
                    }
                    print_virtual_register(vnum, debug_mode);

                } else {
                    // print a real register

                    TargetRegisterInfo* real = &m->target->regclasses[out->required_regclass].regs[out->real];

                    printf(str_fmt, str_arg(real->name));
                }

                skip_until('}', c, i); // skip until }
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