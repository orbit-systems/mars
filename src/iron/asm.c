#include "iron/iron.h"

FeAsm* fe_asm_append(FeModule* m, FeAsm* a) {
    da_append(m->assembly, a);
    return a;
}

void fe_set_target(FeModule* m, u16 arch, u16 system, u16 product) {
    if (!(_FE_ARCH_BEGIN < arch && arch < _FE_ARCH_END))
        CRASH("invalid target architecture");

    if (!(_FE_SYSTEM_BEGIN < system && system < _FE_SYSTEM_END))
        CRASH("invalid target system");

    if (!(_FE_PRODUCT_BEGIN < product && product < _FE_PRODUCT_END))
        CRASH("invalid target product");

    m->target.arch = arch;
    m->target.system = system;
    m->target.product = product;
}

void fe_set_target_config(FeModule* m, void* meta) {
    m->target.arch_config = meta;
}

void fe_codegen(FeModule* m) {
    if (!(_FE_ARCH_BEGIN    < m->target.arch    && m->target.arch    < _FE_ARCH_END)   ||
        !(_FE_SYSTEM_BEGIN  < m->target.system  && m->target.system  < _FE_SYSTEM_END) ||
        !(_FE_PRODUCT_BEGIN < m->target.product && m->target.product < _FE_PRODUCT_END)) {
            CRASH("invalid target");
    }

    

}