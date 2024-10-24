#include "iron/iron.h"
#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){                            \
                                               .function_of_origin = __func__,    \
                                               .message = (msg),                  \
                                               .severity = FE_REP_SEVERITY_FATAL, \
                                           })

u32 fe_mach_get_vreg(FeMachBuffer* buf, FeMachInst* inst, u8 index) {
    return buf->vreg_lists.at[inst->regs + index];
}

void fe_mach_set_vreg(FeMachBuffer* buf, FeMachInst* inst, u8 index, u32 vreg) {
    buf->vreg_lists.at[inst->regs + index] = vreg;
}

static size_t mach_sizes[] = {
    [FE_MACH_SECTION] = sizeof(FeMachSection),
    [FE_MACH_LIFETIME_BEGIN] = sizeof(FeMachLifetimePoint),
    [FE_MACH_LIFETIME_END] = sizeof(FeMachLifetimePoint),

    [FE_MACH_CFG_BEGIN] = sizeof(FeMach),
    [FE_MACH_CFG_END] = sizeof(FeMach),

    [FE_MACH_LABEL_LOCAL] = sizeof(FeMachLocalLabel),
    [FE_MACH_LABEL_GLOBAL] = sizeof(FeMachGlobalLabel),

    [FE_MACH_INST] = sizeof(FeMachInst),

    [_FE_MACH_MAX] = 0,
};

FeMach* fe_mach_new(FeMachBuffer* buf, u8 kind) {
    if (mach_sizes[kind] == 0) {
        CRASH("unknown mach element kind");
    }
    FeMach* m = arena_alloc(&buf->buf_alloca, mach_sizes[kind], 8);
    m->kind = kind;
    return m;
}

FeMach* fe_mach_append(FeMachBuffer* buf, FeMach* inst) {
    da_append(&buf->buf, inst);
    return inst;
}

FeMach* fe_mach_new_lifetime_begin(FeMachBuffer* buf, u32 vreg) {
    FeMachLifetimePoint* m = (FeMachLifetimePoint*)fe_mach_new(buf, FE_MACH_LIFETIME_BEGIN);
    m->vreg = vreg;
    return (FeMach*)m;
}

FeMach* fe_mach_new_lifetime_end(FeMachBuffer* buf, u32 vreg) {
    FeMachLifetimePoint* m = (FeMachLifetimePoint*)fe_mach_new(buf, FE_MACH_LIFETIME_END);
    m->vreg = vreg;
    return (FeMach*)m;
}

FeMachInst* fe_mach_new_inst(FeMachBuffer* buf, u16 template_index) {
    FeMachInst* inst = (FeMachInst*)fe_mach_new(buf, FE_MACH_INST);
    FeMachInstTemplate* templ = &buf->target.inst_templates[template_index];
    inst->template = template_index;
    inst->regs = buf->vreg_lists.len;
    inst->imms = buf->immediates.len;
    if (templ->regs_len != 0) {
        da_reserve(&buf->vreg_lists, templ->regs_len);
        buf->vreg_lists.len += templ->regs_len;
    }
    if (templ->imms_len != 0) {
        da_reserve(&buf->immediates, templ->imms_len);
        buf->immediates.len += templ->imms_len;
    }
    return inst;
}

bool fe_mach_is_vreg_use(FeMachBuffer* buf, u16 template_index, u8 index) {
    return (buf->target.inst_templates[template_index].defs & (1ull << index)) != 0;
}

bool fe_mach_is_vreg_def(FeMachBuffer* buf, u16 template_index, u8 index) {
    return (buf->target.inst_templates[template_index].uses & (1ull << index)) != 0;
}

FeMachImmediate* fe_mach_get_immediate(FeMachBuffer* buf, FeMachImmediateList list, u32 index) {
    u32 imm_index = list + index;
    if (imm_index >= buf->immediates.len) return NULL;
    return &buf->immediates.at[imm_index];
}

FeMachBuffer fe_mach_codegen(FeModule* m) {
    if (m->target.arch == NULL) FE_FATAL(m, "target arch not set");
    if (m->target.system == 0) FE_FATAL(m, "target system not set");

    FeMachBuffer mb = m->target.arch->cg(m);
    return mb;
}

void fe_mach_emit_text(FeDataBuffer* db, FeMachBuffer* mb) {
    mb->target.arch->emit_text(db, mb);
}

u32 fe_mach_new_vreg(FeMachBuffer* buf, u8 regclass) {
    FeMachVReg vreg = {0};
    vreg.class = regclass;
    da_append(&buf->vregs, vreg);
    return buf->vregs.len - 1;
}