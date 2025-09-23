#include "iron/iron.h"

#include "xr17032/xr.h"

#include <stdio.h>

// target construction.
const FeTarget* fe_make_target(FeArch arch, FeSystem system) {
    FeTarget* t = fe_malloc(sizeof(*t));
    memset(t, 0, sizeof(*t));
    t->arch = arch;
    t->system = system;

    switch (arch) {
    case FE_ARCH_X64:
        printf("warning: x64 target is under construction\n");
        t->ptr_ty = FE_TY_I64;
        t->stack_pointer_align = 8;
        break;
    case FE_ARCH_XR17032:
        t->ptr_ty = FE_TY_I32;
        t->stack_pointer_align = 4;
        t->num_regclasses = 2; // counting the NONE regclass
        t->regclass_lens = fe_xr_regclass_lens;
        t->isel = fe_xr_isel;
        t->choose_regclass = fe_xr_choose_regclass;
        t->ir_print_inst = fe_xr_print_inst;
        t->reg_name = fe_xr_reg_name;
        t->reg_status = fe_xr_reg_status;

        fe__load_extra_size_table(FE__XR_INST_BEGIN, fe_xr_extra_size_table, FE__XR_INST_END - FE__XR_INST_BEGIN);
        fe__load_trait_table(FE__XR_INST_BEGIN, fe_xr_trait_table, FE__XR_INST_END - FE__XR_INST_BEGIN);
        break;
    default:
        FE_CRASH("arch unsupported");
    }

    switch (system) {
    case FE_SYSTEM_FREESTANDING:
        break;
    default:
        FE_CRASH("system unsupported");
    }

    return t;
}
