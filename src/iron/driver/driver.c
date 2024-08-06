#define ORBIT_IMPLEMENTATION
#include "iron/iron.h"
#include "common/orbit/orbit_ll.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

ll_typedef(int);

void print_linked_list(ll(int)* n, int pos) {
    if (n->prev && pos <= 0) print_linked_list(n->prev, pos - 1);

    printf("%d %p\n", pos, n);
    printf("    prev %p\n", n->prev);
    printf("    next %p\n", n->next);

    if (n->next && pos >= 0) print_linked_list(n->next, pos + 1);
}

int main() {
    fe_selftest();

    ll(int)* n1 = ll_new(int);
    ll_push_back(n1, ll_new(int));
    ll_push_back(n1, ll_new(int));

    print_linked_list(n1, 0);
    printf("\n");

    ll_remove(n1->next->next);

    print_linked_list(n1, 0);

}