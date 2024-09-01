#include "iron/iron.h"
#include "iron/codegen/mach.h"

FeMachVReg* fe_mach_get_vreg(FeMachBuffer* buf, FeMachVregList list, u64 index) {
    // list is an index into buf.vreg_refs
    // which are indices into buf.vregs
    u32 vreg_ref = list + index;
    if (vreg_ref >= buf->vreg_refs.len) return NULL;
    u32 vreg_index = buf->vreg_refs.at[vreg_ref];
    if (vreg_index >= buf->vregs.len) return NULL;
    return &buf->vregs.at[vreg_index];
}