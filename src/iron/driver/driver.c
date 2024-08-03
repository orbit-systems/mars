#define ORBIT_IMPLEMENTATION
#include "iron/iron.h"
#include "common/orbit/orbit_ll.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

ll_typedef(int);

int main() {
    ll(int)* n = ll_new(int);

    ll_insert_next(n, ll_new(int));

    TODO("iron frontend!");
}