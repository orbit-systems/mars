#include "atlas.h"
#include "passes/passes.h"

#include "targets/aphelion/aphelion.h"

/*
    passes act like a queue. when a pass is about to be run, it is taken off of the queue.

*/

// add a pass so that it runs after all the current passes have ran
void atlas_sched_pass(AtlasModule* m, AtlasPass* p) {
    da_append(&m->pass_queue, p);
}

// if index is 0, it happens next.
// if index >= number of passes scheduled, it runs after all current passes.
void atlas_sched_pass_at(AtlasModule* m, AtlasPass* p, int index) {
    if (index >= m->pass_queue.len) {
        da_append(&m->pass_queue, p);
    } else {
        da_insert_at(&m->pass_queue, p, index);
    }
}

// run the next scheduled pass.
void atlas_run_next_pass(AtlasModule* m, bool printout) {
    if (m->pass_queue.len == 0) return;

    AtlasPass* next = m->pass_queue.at[0];

    if (next->requires_cfg && !m->pass_queue.cfg_up_to_date) {
        atlas_sched_pass_at(m, &air_pass_cfg, 0);
        atlas_run_next_pass(m, printout);
        m->pass_queue.cfg_up_to_date = true;
    }

    da_pop_front(&m->pass_queue);
    
    // call the pass lol
    if (printout) {
        printf("running pass '%s'\n", next->name);
    }
    next->callback(m);

    if (next->modifies_cfg) {
        m->pass_queue.cfg_up_to_date = false;
    }
}

void atlas_run_all_passes(AtlasModule* m, bool printout) {

    while (m->pass_queue.len > 0) {
        if (printout) printstr(air_textual_emit(m));
        atlas_run_next_pass(m, printout);
    }
    if (printout) printstr(air_textual_emit(m));
}