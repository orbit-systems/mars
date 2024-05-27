#include "atlas.h"

/* pass "algsimp" - algebraic simplification and optimization
    (a + 1) + 2  ->  a + (1 + 2)  ->  a + 3

*/

static void reassoc_if_possible(AIR* ir) {

}

AIR_Module* air_pass_algsimp(AIR_Module* mod) {
    return mod;
}