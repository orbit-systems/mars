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

static const u8 mars_paramregs[] = {
    FE_X64_GPR_RDI,
    FE_X64_GPR_RSI,
    FE_X64_GPR_RDX,
    FE_X64_GPR_RCX,
    FE_X64_GPR_R8,
    FE_X64_GPR_R9,
};

static void put_ir_vreg(FeIr* inst, u32 vreg) {
    ptrmap_put(&ir2vreg, inst, (void*)(u64)vreg);
}

static u32 get_ir_vreg(FeIr* inst) {
    return (u32)(u64) ptrmap_get(&ir2vreg, inst);
}

static void gen_basic_block(FeMachBuffer* buf, FeFunction* fn, FeBasicBlock* bb, bool entry) {
    if (entry) {
        size_t numparams = bb->function->params.len;
        u32* param_vregs = fe_malloc(sizeof(param_vregs[0]) * numparams);

        assert(numparams <= 6);

        FeIr* inst = bb->start;
        while (inst->kind == FE_IR_PARAMVAL) {
            FeIrParamVal* pv = (FeIrParamVal*) inst;
            u32 param_reg = fe_mach_new_vreg(buf, FE_X64_REGCLASS_GPR);
            buf->vregs.at[param_reg].real = mars_paramregs[pv->index];
            param_vregs[pv->index] = param_reg;
            
            // add register lifetimes
            fe_mach_append(buf, fe_mach_new_lifetime_begin(buf, param_reg));
            inst = inst->next;
        }

        inst = bb->start;
        while (inst->kind == FE_IR_PARAMVAL) {
            FeIrParamVal* pv = (FeIrParamVal*) inst;

            u32 paramval_out_reg = fe_mach_new_vreg(buf, FE_X64_REGCLASS_GPR);
            put_ir_vreg(inst, paramval_out_reg);
            buf->vregs.at[paramval_out_reg].hint = mars_paramregs[pv->index];

            FeMachInst* mov = fe_mach_new_inst(buf, FE_X64_INST_MOV_RR_64);
            fe_mach_set_vreg(buf, mov, 0, paramval_out_reg);
            fe_mach_set_vreg(buf, mov, 1, param_vregs[pv->index]);

            fe_mach_append(buf, (FeMach*) mov);

            inst = inst->next;
        }


        fe_free(param_vregs);
    }
}

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

    assert(fn->blocks.len == 1);
    gen_basic_block(buf, fn, fn->blocks.at[0], true);

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
    da_init(&mb.vregs, 512);
    da_init(&mb.vreg_lists, 512);
    mb.buf_alloca = arena_make(1024);

    gen_symtab(mod, &mb);

    for_range(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        gen_function(&mb, fn);
    }

    // TODO("");

    return mb;
}