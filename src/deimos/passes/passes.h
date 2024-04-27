#pragma once
#define DEIMOS_PASSES_H

#include "deimos.h"
#include "ir.h"

IR_Module* ir_generate(mars_module* mod);

IR_Module* ir_pass_stackorg(IR_Module* mod);

/* REQIRED PASSES
    stackorg        reoganize stackallocs
*/

/* OPTIMIZATION PASSES - optional
    algsimp         algebraic simplification
    dce             dead code elimination
    gvn             global value numbering
    stack promote   promotion of memory to registers
    sroa            scalar replacement of aggregates
*/