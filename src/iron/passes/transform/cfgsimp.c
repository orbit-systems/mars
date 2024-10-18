#include "iron/iron.h"

/* pass "cfgsimp" - control flow graph simplification

    branch with matching destinations -> jump
    blocks with no predecessors get removed
    branch with constant condition -> jump
    block with 1 successor jumps to block with 1 predecessor get merged

*/