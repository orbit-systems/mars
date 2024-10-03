#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"
#include "common/ptrmap.h"

// ir -> x64 mach ir

PtrMap irsym2machsym;

static u32 mach_symbol(FeSymbol* sym) {
    return (u32)(u64) ptrmap_get(&irsym2machsym, sym);
}

// generate symbol table and predefs/bindings if needed
static void gen_symtab(FeModule* m, FeMachBuffer* mb) {
    if (irsym2machsym.keys == NULL) {
        ptrmap_init(&irsym2machsym, m->symtab.len * 2);
    }

    foreach (FeSymbol* ir_sym, m->symtab) {
        FeMachSymbol msym = {};
        msym.binding = ir_sym->binding;
        msym.name = ir_sym->name.raw;
        msym.name_len = ir_sym->name.len;
        da_append(&mb->symtab, msym);
        ptrmap_put(&irsym2machsym, ir_sym, (void*)(mb->symtab.len - 1));
    }
}

PtrMap ir2vreg;

static void gen_function(FeMachBuffer* buf, FeFunction* fn) {

    // only support mars callconv rn
    if (fn->cconv != FE_CALLCONV_MARS) {
        CRASH("only support mars callconv rn");
    }

    if (ir2vreg.keys == NULL) {
        ptrmap_init(&ir2vreg, 512);
    }
    ptrmap_reset(&ir2vreg);

    // generate function header
    FeMachLabel* head_label = (FeMachLabel*) fe_mach_append(buf, fe_mach_new(buf, FE_MACH_LABEL_GLOBAL));
    head_label->symbol_index = mach_symbol(fn->sym);

    fe_mach_append(buf, fe_mach_new(buf, FE_MACH_REGALLOC_BEGIN));

    fe_mach_append(buf, fe_mach_new(buf, FE_MACH_REGALLOC_END));
}

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
    da_init(&mb.symtab, mod->symtab.len);
    mb.buf_alloca = arena_make(1024);

    gen_symtab(mod, &mb);

    for_range(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        gen_function(&mb, fn);
    }

    // TODO("");

    return mb;
}