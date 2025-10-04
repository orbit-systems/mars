// binary generation

#include "iron/iron.h"
#include "xr.h"
#include "../mir/mir.h"

u8 get_register(FeFunc* f, FeInst* inst) {
    return f->vregs->at[inst->vr_def].real;
}
