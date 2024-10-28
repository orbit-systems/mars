#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"
#include "common/ptrmap.h"

// ir -> x64 mach ir

PtrMap irsym2machsym;

static u32 mach_symbol(FeSymbol* sym) {
    return (u32)(u64)ptrmap_get(&irsym2machsym, sym);
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

static void put_ir_vreg(FeIr* inst, u32 vreg) {
    ptrmap_put(&ir2vreg, inst, (void*)(u64)vreg);
}

static u32 get_ir_vreg(FeIr* inst) {
    void* r = ptrmap_get(&ir2vreg, inst);
    return r != PTRMAP_NOT_FOUND ? (u32)(u64)r : 0;
}

static bool does_ir_have_vreg(FeIr* inst) {
    return ptrmap_get(&ir2vreg, inst) == PTRMAP_NOT_FOUND;
}

static const u8 mars_cconv_paramregs[] = {
    FE_X64_GPR_RDI,
    FE_X64_GPR_RSI,
    FE_X64_GPR_RDX,
    FE_X64_GPR_RCX,
    FE_X64_GPR_R8,
    FE_X64_GPR_R9,
};

static const u8 mars_cconv_returnregs[] = {
    FE_X64_GPR_R9,
    FE_X64_GPR_R8,
    FE_X64_GPR_RCX,
    FE_X64_GPR_RDX,
    FE_X64_GPR_RSI,
    FE_X64_GPR_RDI,
};

#define new_inst(b, kind) (FeMachInst*)fe_mach_append(buf, (FeMach*)fe_mach_new_inst(buf, kind))

static FeIr* emit_prologue(FeMachBuffer* buf, FeFunction* fn, FeIr* ir) {
    u32* callconv_vregs = fe_malloc(sizeof(u32) * fn->params.len);
    switch (fn->cconv) {
    case FE_CCONV_MARS:
        assert(fn->params.len <= 6); // preparation passes should have made this never fail
        for (FeIrParam* pv = (FeIrParam*)ir; pv->base.kind == FE_IR_PARAM; pv = (FeIrParam*)pv->base.next) {
            // create the vreg for the callconv register
            u32 cconv_vreg = fe_mach_new_vreg(buf, FE_X64_REGCLASS_GPR);
            callconv_vregs[pv->index] = cconv_vreg;
            buf->vregs.at[cconv_vreg].real = mars_cconv_paramregs[pv->index];
            // begin its lifetime
            fe_mach_append(buf, fe_mach_new_lifetime_begin(buf, cconv_vreg));
        }
        for (FeIrParam* pv = (FeIrParam*)ir; pv->base.kind == FE_IR_PARAM; pv = (FeIrParam*)pv->base.next) {
            // create the mov
            FeMachInst* mov = new_inst(buf, FE_X64_INST_MOV_RR_64);
            fe_mach_set_vreg(buf, mov, 1, callconv_vregs[pv->index]);

            u32 mov_out = fe_mach_new_vreg(buf, FE_X64_REGCLASS_GPR);
            buf->vregs.at[mov_out].hint = mars_cconv_paramregs[pv->index];
            fe_mach_set_vreg(buf, mov, 0, mov_out);
            put_ir_vreg((FeIr*)pv, mov_out);
            ir = (FeIr*)pv;
        }
        break;
    default:
        TODO("non-mars cconvs not supported yet");
        break;
    }
    fe_free(callconv_vregs);
    return ir;
}

static FeIr* emit_epilogue(FeMachBuffer* buf, FeFunction* fn, FeIr* ir) {
    u32* callconv_vregs = fe_malloc(sizeof(u32) * fn->returns.len);

    FeIrReturn* ret = (FeIrReturn*)ir;
    for_range(i, 0, ret->len) {
        // create the vreg for the callconv register
        u32 cconv_vreg = fe_mach_new_vreg(buf, FE_X64_REGCLASS_GPR);
        callconv_vregs[i] = cconv_vreg;
        buf->vregs.at[cconv_vreg].real = mars_cconv_returnregs[i];

        // generate the mov
        FeMachInst* mov = new_inst(buf, FE_X64_INST_MOV_RR_64);
        fe_mach_set_vreg(buf, mov, 1, get_ir_vreg(ret->sources[i]));
        fe_mach_set_vreg(buf, mov, 0, cconv_vreg);
    }

    for_range(i, 0, ret->len) {
        // generate the lifetime endpoints
        fe_mach_append(buf, fe_mach_new_lifetime_end(buf, callconv_vregs[i]));
    }

    fe_free(callconv_vregs);
    return ir;
}

static void gen_basic_block(FeMachBuffer* buf, FeBasicBlock* bb) {
    // generate block label
    FeMachLocalLabel* block_label = (FeMachLocalLabel*)fe_mach_append(buf, fe_mach_new(buf, FE_MACH_LABEL_LOCAL));
    block_label->name = bb->name;

    FeMachInst* minst;
    for_fe_ir(ir, *bb) switch (ir->kind) {
    case FE_IR_PARAM:
        ir = emit_prologue(buf, bb->function, ir);
        break;
    case FE_IR_RETURN:
        ir = emit_epilogue(buf, bb->function, ir);
        new_inst(buf, FE_X64_INST_RET);
        break;
    case FE_IR_ADD:
        FeIrBinop* binop = (FeIrBinop*)ir;
        // insert a mov to move the first operand into the lhs
        u32 lhs_vreg;
        if (binop->lhs->flags != 1) {
            lhs_vreg = fe_mach_new_vreg(buf, FE_X64_REGCLASS_GPR);
            FeMachInst* mov = new_inst(buf, FE_X64_INST_MOV_RR_64);
            fe_mach_set_vreg(buf, mov, 0, lhs_vreg);
            fe_mach_set_vreg(buf, mov, 1, get_ir_vreg(binop->lhs));
        } else {
            lhs_vreg = get_ir_vreg(binop->lhs);
        }

        switch (ir->type) {
        case FE_TYPE_I64: minst = new_inst(buf, FE_X64_INST_ADD_RR_64); break;
        default: TODO("");
        }

        fe_mach_set_vreg(buf, minst, 0, lhs_vreg);
        fe_mach_set_vreg(buf, minst, 1, get_ir_vreg(binop->rhs));
        put_ir_vreg(ir, lhs_vreg);
        break;
    default:
        TODO("unhandled ir");
    }
}

static void gen_function(FeMachBuffer* buf, FeFunction* fn) {

    // init ir2vreg ptrmap
    if (ir2vreg.keys == NULL) ptrmap_init(&ir2vreg, 512);
    ptrmap_reset(&ir2vreg);

    // generate function header
    FeMachGlobalLabel* head_label = (FeMachGlobalLabel*)fe_mach_append(buf, fe_mach_new(buf, FE_MACH_LABEL_GLOBAL));
    head_label->symbol_index = mach_symbol(fn->sym);
    fe_mach_append(buf, fe_mach_new(buf, FE_MACH_CFG_BEGIN));

    foreach (FeBasicBlock* bb, fn->blocks) {
        gen_basic_block(buf, bb);
    }

    fe_mach_append(buf, fe_mach_new(buf, FE_MACH_CFG_END));
}

static FeMachVReg* get_vreg(FeMachBuffer* buf, FeMachInst* inst, u32 i) {
    return &buf->vregs.at[buf->vreg_lists.at[inst->regs + i]];
}

static void mov_reduce(FeMachBuffer* buf) {
    for_range(i, 0, buf->buf.len) {
        if (buf->buf.at[i]->kind != FE_MACH_INST)
            continue;
        FeMachInst* inst = (FeMachInst*)buf->buf.at[i];
        if (inst->template != FE_X64_INST_MOV_RR_64)
            continue;

        u8 real0 = get_vreg(buf, inst, 0)->real;
        if (real0 != 0 && real0 == get_vreg(buf, inst, 1)->real) {
            inst->base.kind = FE_MACH_NONE;
        }
    }
}

FeMachBuffer fe_x64_codegen(FeModule* mod) {
    mod->pass_queue.len = 0; // clear pass queue

    // run preparation passes
    fe_sched_module_pass(mod, &fe_pass_moviphi);
    fe_sched_module_pass(mod, &fe_pass_tdce);
    fe_run_all_passes(mod, false);

    FeMachBuffer mb = {0};

    mb.target.arch = mod->target.arch;
    mb.target.arch_config = mod->target.arch_config;
    mb.target.system = mod->target.system;
    mb.target.system_config = mod->target.system_config;
    mb.target.inst_templates = fe_x64_inst_templates;

    da_init(&mb.buf, 256);
    da_init(&mb.immediates, 128);
    da_init(&mb.symtab, mod->symtab.len);
    da_init(&mb.vregs, 512);
    fe_mach_new_vreg(&mb, FE_X64_REGCLASS_UNKNOWN); // add null register
    da_init(&mb.vreg_lists, 512);
    mb.buf_alloca = arena_make(1024);

    gen_symtab(mod, &mb);

    for_range(i, 0, mod->functions_len) {
        FeFunction* fn = mod->functions[i];
        gen_function(&mb, fn);
    }

    // printstr();
    FeDataBuffer db = fe_db_new(128);

    fe_mach_emit_text(&db, &mb);

    printf("\n%s\n", fe_db_clone_to_cstring(&db));

    // TODO("");
    printf("register allocation!\n");
    printf("live ranges:\n");
    fe_mach_regalloc(&mb);

    printf("reducing redundant movs...\n");
    mov_reduce(&mb);

    // printf("final code generation!\n");

    return mb;
}