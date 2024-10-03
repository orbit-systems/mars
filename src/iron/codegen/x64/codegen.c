#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"

// ir -> mach ir

FeMachBuffer fe_x64_codegen(FeModule* mod) {
    FeMachBuffer mb = {0};
    if (mod->target.arch != FE_ARCH_X64) {
        CRASH("fe_x64_codegen called with arch != FE_ARCH_X64"); // make fatal report
    }

    mb.target.arch = mod->target.arch;
    mb.target.arch_config = mod->target.arch_config;
    mb.target.system = mod->target.system;
    mb.target.system_config = mod->target.system_config;
    mb.target.inst_templates = fe_x64_inst_templates;

    da_init(&mb.buf, 256);
    da_init(&mb.immediates, 128);
    mb.buf_alloca = arena_make(1024);
    
    return mb;
}