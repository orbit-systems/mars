#include "iron.h"

FeAsm* fe_asm_append(FeModule* m, FeAsm* a) {
    da_append(m->assembly, a);
    return a;
}