#include "exactval.h"
#include "arena.h"

exact_value* new_exact_value(int aggregate_len, arena* restrict alloca) {
    exact_value* ev;
    if (alloca == NULL) return NULL;
    if (alloca == USE_MALLOC) {
        ev = malloc(sizeof(exact_value));
        ev->freeable = true;
    } else {
        ev = arena_alloc(alloca, sizeof(exact_value), alignof(exact_value));
    }
    memset(ev, 0, sizeof(*ev));
    if (aggregate_len != NO_AGGREGATE) {
        ev->as_aggregate.len = aggregate_len;
        if (alloca == USE_MALLOC)
            ev->as_aggregate.vals = malloc(sizeof(exact_value*) * aggregate_len);
        else
            ev->as_aggregate.vals = arena_alloc(alloca, sizeof(exact_value*) * aggregate_len, alignof(exact_value*));
        memset(ev->as_aggregate.vals, 0, sizeof(exact_value*) * aggregate_len);
    }
    return ev;

}

void destroy_exact_value(exact_value* ev) {
    if (ev == NULL) return;
    if (!ev->freeable) return;
    free(ev->as_aggregate.vals);
    free(ev);
}