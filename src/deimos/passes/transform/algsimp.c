#include "deimos.h"

/* pass "algsimp" - algebraic simplification and optimization
    (a + 1) + 2  ->  a + (1 + 2)  ->  a + 3

*/

static void reassoc_if_possible(IR* ir) {

}

IR_Module* ir_pass_algsimp(IR_Module* mod) {
    return mod;
}