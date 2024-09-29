#include "iron/iron.h"
#include "iron/codegen/mach.h"

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

FeMachBuffer fe_mach_new_buffer(FeModule* m) {
    
}