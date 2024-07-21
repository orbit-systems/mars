#include "iron/iron.h"
#include "passes/passes.h"

#include "targets/aphelion/aphelion.h"

/*
    passes act like a queue. when a pass is about to be run, it is taken off of the queue.
*/

// add a pass so that it runs after all the current passes have ran
void fe_sched_pass(FeModule* m, FePass* p) {
    da_append(&m->pass_queue, p);
}

// if index is 0, it happens next.
// if index >= number of passes scheduled, it runs after all current passes.
void fe_sched_pass_at(FeModule* m, FePass* p, int index) {
    if (index >= m->pass_queue.len) {
        da_append(&m->pass_queue, p);
    } else {
        da_insert_at(&m->pass_queue, p, index);
    }
}

// run the next scheduled pass.
void fe_run_next_pass(FeModule* m, bool printout) {
    if (m->pass_queue.len == 0) return;

    FePass* next = m->pass_queue.at[0];

    if (next->requires_cfg && !m->pass_queue.cfg_up_to_date) {
        fe_sched_pass_at(m, &fe_pass_cfg, 0);
        fe_run_next_pass(m, printout);
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

void fe_run_all_passes(FeModule* m, bool printout) {

    while (m->pass_queue.len > 0) {
        if (printout) printstr(fe_emit_textual_ir(m));
        fe_run_next_pass(m, printout);
    }
    if (printout) printstr(fe_emit_textual_ir(m));
}