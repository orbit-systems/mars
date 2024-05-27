#pragma once
#define DEIMOS_PASSES_H

#include "deimos.h"
#include "ir.h"

AIR_Module* air_generate(mars_module* mod);

/* REQIRED PASSES
    canon           general cleanup & canonicalization
*/

AIR_Module* air_pass_canon(AIR_Module* mod);

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

AIR_Module* air_pass_movprop(AIR_Module* mod);
AIR_Module* air_pass_elim(AIR_Module* mod);
AIR_Module* air_pass_tdce(AIR_Module* mod);
AIR_Module* air_pass_trme(AIR_Module* mod);