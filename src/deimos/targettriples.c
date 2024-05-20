#include "targettriples.h"

void set_target_triple(string target, flag_set* fl) {
    
    int first_dash = 0;
    for (; first_dash < target.len; first_dash++) {
        if (target.raw[first_dash] == '-' ) {
            break;
        }
    }
    if (first_dash == 0 || first_dash == target.len) general_error("invalid target '"str_fmt"'", str_arg(target));

    int second_dash = first_dash + 1;
    for (; second_dash < target.len; second_dash++) {
        if (target.raw[second_dash] == '-') {
            break;
        }
    }
    if (second_dash == first_dash + 1 || second_dash == target.len) general_error("invalid target "str_fmt"'", str_arg(target));

    string arch    = substring(target, 0, first_dash);
    string sys     = substring(target, first_dash + 1, second_dash);
    string product = substring(target, second_dash + 1, target.len);

    // LMFAO

    fl->target_arch = -1;
    fl->target_system = -1;
    fl->target_product = -1;

    #define X(variant, str) if (string_eq(arch, constr(str))) fl->target_arch = TARGET_ARCH_##variant;
        SUPPORTED_ARCH
    #undef X

    #define X(variant, str) if (string_eq(sys, constr(str))) fl->target_system = TARGET_SYS_##variant;
        SUPPORTED_SYS
    #undef X

    #define X(variant, str) if (string_eq(product, constr(str))) fl->target_product = TARGET_PRODUCT_##variant;
        SUPPORTED_PRODUCT
    #undef X

    if (fl->target_arch == -1)    general_error("unknown target architecture '"str_fmt"'", str_arg(arch));
    if (fl->target_system == -1)  general_error("unknown target system '"str_fmt"'", str_arg(sys));
    if (fl->target_product == -1) general_error("unknown target product '"str_fmt"'", str_arg(product));
    
}