#include "exactval.h"
#include "arena.h"
#include "alloc.h"

// TODO rework exactvals to use a type* instead of a kind tag.
// it sucks to keep the 'exact_value's and the 'checked_expr's synced up

exact_value* alloc_exact_value(int aggregate_len, arena* alloca) {
    exact_value* ev;
    if (alloca == NULL) return NULL;
    if (alloca == USE_MALLOC) {
        ev = mars_alloc(sizeof(exact_value));
        ev->freeable = true;
    } else {
        ev = arena_alloc(alloca, sizeof(exact_value), alignof(exact_value));
    }
    memset(ev, 0, sizeof(*ev));
    if (aggregate_len != NO_AGGREGATE) {
        ev->as_aggregate.len = aggregate_len;
        if (alloca == USE_MALLOC)
            ev->as_aggregate.vals = mars_alloc(sizeof(exact_value*) * aggregate_len);
        else
            ev->as_aggregate.vals = arena_alloc(alloca, sizeof(exact_value*) * aggregate_len, alignof(exact_value*));
        memset(ev->as_aggregate.vals, 0, sizeof(exact_value*) * aggregate_len);
    }
    return ev;
}

void destroy_exact_value(exact_value* ev) {
    if (ev == NULL) return;
    if (!ev->freeable) return;
    mars_free(ev->as_aggregate.vals);
    mars_free(ev);
}

exact_value* copy_ev_to_permanent(exact_value* ev) {
    exact_value* new = mars_alloc(sizeof(exact_value));
    memcpy(new, ev, sizeof(exact_value));
    new->freeable = true;
    return new;
}