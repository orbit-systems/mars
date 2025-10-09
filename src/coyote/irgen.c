#include "irgen.h"
#include "parse.h"

FeModule* irgen(CompilationUnit* cu) {

    FeModule* mod = fe_module_new(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    printf("FUNCTIONS:\n");
    for_n(i, 0, cu->top_scope->map.cap) {
        Entity* e = cu->top_scope->map.vals[i];
        if (e == nullptr) {
            continue;
        }
        if (e->kind != ENTKIND_FN) {
            continue;
        }
        printf("    "str_fmt"\n", str_arg(from_compact(e->name)));
    }

    printf("STATIC VARIABLES:\n");
    for_n(i, 0, cu->top_scope->map.cap) {
        Entity* e = cu->top_scope->map.vals[i];
        if (e == nullptr) {
            continue;
        }
        if (e->kind != ENTKIND_VAR) {
            continue;
        }
        printf("    "str_fmt"\n", str_arg(from_compact(e->name)));
    }

    printf("TYPES:\n");
    for_n(i, 0, cu->top_scope->map.cap) {
        Entity* e = cu->top_scope->map.vals[i];
        if (e == nullptr) {
            continue;
        }
        if (e->kind != ENTKIND_TYPE) {
            continue;
        }
        printf("    "str_fmt"\n", str_arg(from_compact(e->name)));
    }

    printf("%d symbols\n%d capacity\n", cu->top_scope->map.size, cu->top_scope->map.cap);

    return mod;
}
