#include "iron/iron.h"
#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){ \
    .function_of_origin = __func__,\
    .message = (msg),\
    .severity = FE_MSG_SEVERITY_FATAL, \
})

FeMachVReg* fe_mach_get_vreg(FeMachBuffer* buf, FeMachVregList list, u32 index) {
    // list is an index into buf.vreg_refs
    // which are indices into buf.vregs
    u32 vreg_ref = list + index;
    if (vreg_ref >= buf->vreg_refs.len) return NULL;
    u32 vreg_index = buf->vreg_refs.at[vreg_ref];
    if (vreg_index >= buf->vregs.len) return NULL;
    return &buf->vregs.at[vreg_index];
}

FeMachImmediate* fe_mach_get_immediate(FeMachBuffer* buf, FeMachImmediateList list, u32 index) {
    u32 imm_index = list + index;
    if (imm_index >= buf->immediates.len) return NULL;
    return &buf->immediates.at[imm_index];
}

FeMachBuffer fe_mach_codegen(FeModule* m) {
    if (m->target.arch == 0) FE_FATAL(m, "target arch not set");
    if (m->target.system == 0) FE_FATAL(m, "target system not set");

    TODO("");
}

u32 fe_mach_new_vreg(FeMachBuffer* buf, u16 regclass) {
    FeMachVReg vreg = {0};
    vreg.class = regclass;
    da_append(&buf->vregs, vreg);
    return buf->vregs.len - 1;
}

FeMachInstTemplate* fe_mach_get_inst_template(FeMachBuffer* buf, u16 inst_code) {
    switch (buf->arch) {
    case FE_ARCH_X64:
        return (inst_code >= _FE_X64_INST_MAX) ? NULL : &fe_x64_inst_templates[inst_code];
    default:
        CRASH("");
        break;
    }
}

FeMach* fe_mach_get(FeMachBuffer* buf, u32 index) {
    return buf->buf.at[index];
}