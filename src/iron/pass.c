#include "iron/iron.h"
#include "passes/passes.h"

/*
    passes act like a queue. when a pass is about to be run, it is taken off of the queue.
*/

void fe_sched_func_pass(FeModule* m, FePass* p, FeFunction* fn) {
    FeSchedPass sp;
    sp.sched_kind = FE_SCHED_FUNCTION;
    sp.bind.fn = fn;
    sp.pass = p;
    da_append(&m->pass_queue, sp);
}

void fe_sched_module_pass(FeModule* m, FePass* p) {
    FeSchedPass sp;
    sp.sched_kind = FE_SCHED_MODULE;
    sp.pass = p;
    da_append(&m->pass_queue, sp);
}

void fe_sched_func_pass_at(FeModule* m, FePass* p, FeFunction* fn, u64 index) {
    FeSchedPass sp;
    sp.sched_kind = FE_SCHED_FUNCTION;
    sp.bind.fn = fn;
    sp.pass = p;
    da_insert_at(&m->pass_queue, sp, index);
}

void fe_sched_module_pass_at(FeModule* m, FePass* p, u64 index) {
    FeSchedPass sp;
    sp.sched_kind = FE_SCHED_MODULE;
    sp.pass = p;
    da_insert_at(&m->pass_queue, sp, index);
}

void fe_run_all_passes(FeModule* m, bool printout) {

    string text;
    while (m->pass_queue.len > 0) {
        if (printout) {
            text = fe_emit_ir(m);
            printstr(text);
            string_free(text);
        }
        fe_run_next_pass(m);
    }
    if (printout) {
        text = fe_emit_ir(m);
        printstr(text);
        string_free(text);
    }
}

void fe_run_next_pass(FeModule* m) {

    FeSchedPass sp = m->pass_queue.at[0];
    da_pop_front(&m->pass_queue);

    printf("running pass '%s'...\n", sp.pass->name);

    switch (sp.sched_kind) {
    case FE_SCHED_FUNCTION:
        sp.pass->function(sp.bind.fn);
        break;
    case FE_SCHED_MODULE:
        sp.pass->module(m);
    default:
        break;
    }

}