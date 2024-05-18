#include "targettriples.h"

int arch_from_str(string arch) {
    if (string_eq(arch, str("aphelion"))) return 0;

    return -1;
}

int sys_from_str(string sys) {
    if (string_eq(sys, str("unknown")))      return 0;
    if (string_eq(sys, str("freestanding"))) return 0;
    return -1;
}

int product_from_str(string prod) {
    if (string_eq(prod, str("asm"))) return 0;

    return -1;
}
