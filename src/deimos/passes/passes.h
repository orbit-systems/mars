#pragma once
#define DEIMOS_PASSES_H

#include "deimos.h"
#include "ir.h"

IR_Module* ir_generate(mars_module* mod);

/* REQIRED PASSES
    canon           general cleanup & canonicalization
*/

IR_Module* ir_pass_canon(IR_Module* mod);

/* ANALYSIS PASSES - provides/populates information about the code

*/

/* OPTIMIZATION PASSES - optional
    movprop         mov propogation
    elim            remove instructions marked eliminated
    tdce            trivial dead code elimination
    trme            trivial redundant memory op elimination

(TODO)
    algsimp         algebraic simplification
    dce             dead code elimination
    gvn             global value numbering
    stackpromote    promotion of memory to registers
    sroa            scalar replacement of aggregates
*/

IR_Module* ir_pass_movprop(IR_Module* mod);
IR_Module* ir_pass_elim(IR_Module* mod);
IR_Module* ir_pass_tdce(IR_Module* mod);
IR_Module* ir_pass_trme(IR_Module* mod);