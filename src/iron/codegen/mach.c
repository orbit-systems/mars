#include "iron/iron.h"
#include "iron/codegen/mach.h"
#include "iron/codegen/x64/x64.h"

#define FE_FATAL(m, msg) fe_push_report(m, (FeReport){ \
    .function_of_origin = __func__,\
    .message = (msg),\
    .severity = FE_MSG_SEVERITY_FATAL, \
})

FeMachVReg* fe_mach_get_vreg(FeMachBuffer* buf, FeMachVregList list, u32 index) {
    
    return &buf->vregs.at[list + index];
}

bool fe_mach_is_vreg_use(FeMachBuffer* buf, FeMachInstTemplateIndex template, u8 index) {
    return (buf->target.inst_templates[template].defs & (1ull << index)) != 0;
}

bool fe_mach_is_vreg_def(FeMachBuffer* buf, FeMachInstTemplateIndex template, u8 index) {
    return (buf->target.inst_templates[template].uses & (1ull << index)) != 0;
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

FeMach* fe_mach_get(FeMachBuffer* buf, u32 index) {
    return buf->buf.at[index];
}