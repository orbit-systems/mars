#pragma once

#include "iron/iron.h"

/* ANALYSIS PASSES
    cfg             populate and provide information about control flow graphs
*/

extern FePass fe_pass_cfg;

/* OPTIMIZATION PASSES
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

extern FePass fe_pass_movprop;
extern FePass fe_pass_elim;
extern FePass fe_pass_tdce;
extern FePass fe_pass_algsimp;
extern FePass fe_pass_stackprom;