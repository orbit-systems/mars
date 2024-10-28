#include "iron/iron.h"

/* pass "sccp" - sparse conditional constant propagation

    TODO

    performs a limited abstract interpretation to determine if values are
    constant (better than algsimp can). This also modifies the CFG if it
    detects a branch on a constant.

*/

enum {
    SV_UNDEF = 0,
    SV_OVERDEF,
    SV_CONST, // anything equal or over this is constant
};

static struct {
    FeIr** at;
    usize len;
    usize cap;
} const_defs;

