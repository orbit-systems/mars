#include "iron/iron.h"

/* pass "sccp" - sparse conditional constant propagation

    performs a limited abstract interpretation to determine if values are 
    constant (better than algsimp can). This also

    it's a good idea to run a DCE pass cause this can generate some constant garbage.

*/

enum {
    SV_UNDEF = 0,
    SV_OVERDEF,
    SV_CONST,
};

typedef struct SccpValue {
    u32 kind : 2;
    u32 cval : 30; // index
} SccpValue;