#include "iron/iron.h"
#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){                            \
                                               .function_of_origin = __func__,    \
                                               .message = (msg),                  \
                                               .severity = FE_MSG_SEVERITY_FATAL, \
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
    if (m->target.arch == 0) FE_FATAL(m, "target arch not set");
    if (m->target.system == 0) FE_FATAL(m, "target system not set");

    FeMachBuffer mb;
    switch (m->target.arch) {
    case FE_ARCH_X64:
        mb = fe_x64_codegen(m);
        break;
    default:
        FE_FATAL(m, "unsupported architecture");
    }

    // TODO("");
    return mb;
}

u32 fe_mach_new_vreg(FeMachBuffer* buf, u8 regclass) {
    FeMachVReg vreg = {0};
    vreg.class = regclass;
    da_append(&buf->vregs, vreg);
    return buf->vregs.len - 1;
}

// "native" (pointer-sized) integer type for an architecture.
FeType fe_mach_type_of_native_int(u16 arch) {
    switch (arch) {
    case FE_ARCH_X64: return FE_TYPE_I64;
    case FE_ARCH_APHELION: return FE_TYPE_I64;
    case FE_ARCH_ARM64: return FE_TYPE_I64;
    case FE_ARCH_XR17032: return FE_TYPE_I32;
    case FE_ARCH_FOX32: return FE_TYPE_I32;
    default: CRASH("unknown arch");
    }
}

// highest precision floating point format less than or equal in size
// to the arch's native integer type
// returns FE_TYPE_VOID if there's no native floating-point
FeType fe_mach_type_of_native_float(u16 arch) {
    switch (arch) {
    case FE_ARCH_X64: return FE_TYPE_F64;
    case FE_ARCH_APHELION: return FE_TYPE_F64;
    case FE_ARCH_ARM64: return FE_TYPE_F64;
    case FE_ARCH_XR17032: return FE_TYPE_VOID;
    case FE_ARCH_FOX32: return FE_TYPE_VOID;
    default: CRASH("unknown arch");
    }
}

// returns true if this type has a "native" representation on this architecture
// ie. it does not need to be emulated via software or additional fuckery
// returns false for aggregate types
bool fe_mach_type_is_native(u16 arch, FeType t) {
    switch (arch) {
    case FE_ARCH_X64:
        // all types natively supported
        return t <= FE_TYPE_F64;
    case FE_ARCH_APHELION:
        // all types natively supported
        return t <= FE_TYPE_F64;
    case FE_ARCH_ARM64:
        // all types natively supported
        return t <= FE_TYPE_F64;
    case FE_ARCH_XR17032:
        // only integers up to i32 are native
        return t <= FE_TYPE_I32;
    case FE_ARCH_FOX32:
        // only integers up to i32 are native
        return t <= FE_TYPE_I32;
    default: CRASH("unknown arch");
    }
    return false;
}