#pragma once
#define ATLAS_PASSES_H

#include "atlas.h"
#include "ir.h"

/* REQUIRED PASSES
    canon           general cleanup & canonicalization
*/

extern AtlasPass air_pass_canon;

/* ANALYSIS PASSES
    cfg             populate and provide information about control flow graphs
*/

extern AtlasPass air_pass_cfg;

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

extern AtlasPass air_pass_movprop;
extern AtlasPass air_pass_elim;
extern AtlasPass air_pass_tdce;
extern AtlasPass air_pass_trme;