#pragma once

#include "iron/iron.h"

/* ANALYSIS PASSES
    cfg             populate and provide information about control flow graphs
*/

extern FePass fe_pass_cfg;

/* OPTIMIZATION PASSES
    movprop         mov propogation
    algsimp         algebraic simplification
    tdce            trivial dead code elimination
    stackprom       promotion of memory to registers

(TODO)
    dce             dead code elimination
    gvn             global value numbering
    sroa            scalar replacement of aggregates
*/

extern FePass fe_pass_verify;
extern FePass fe_pass_moviphi;
extern FePass fe_pass_movprop;
extern FePass fe_pass_tdce;
extern FePass fe_pass_algsimp;
extern FePass fe_pass_stackprom;