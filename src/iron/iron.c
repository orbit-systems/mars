#include "iron/iron.h"
#include "passes/passes.h"

void fe_typegraph_init(FeModule* m);

FeModule* fe_new_module(string name) {
    FeModule* mod = mars_alloc(sizeof(*mod));

    mod->name = name;

    da_init(&mod->symtab, 32);

    mod->assembly = mars_alloc(sizeof(*mod->assembly));

    da_init(mod->assembly, 32);

    fe_typegraph_init(mod);

    da_init(&mod->pass_queue, 4);
    fe_sched_pass(mod, &fe_pass_canon);

    return mod;
}

// destroy, deallocate, cleanup everything
void fe_destroy_module(FeModule* m) {
    // destroy symbol table.
    arena_delete(&m->symtab.alloca);
    da_destroy(&m->symtab);
    
    // destroy typegraph
    arena_delete(&m->typegraph.alloca);
    da_destroy(&m->typegraph);

    for_urange (i, 0, m->functions_len) {
        fe_destroy_function(m->functions[i]);
    }
    mars_free(m->functions);

    mars_free(m->datas);

    da_destroy(&m->pass_queue);
}

void fe_destroy_function(FeFunction* f) {
    mars_free(f->params);
    mars_free(f->returns);
    da_destroy(&f->stack);
    foreach(FeBasicBlock* bb, f->blocks) {
        fe_destroy_basic_block(bb);
    }
    da_destroy(&f->blocks);
    arena_delete(&f->alloca);
    *f = (FeFunction){0};
}

void fe_destroy_basic_block(FeBasicBlock* bb) {
    mars_free(bb->domset);
    mars_free(bb->incoming);
    mars_free(bb->outgoing);
    da_destroy(bb);

    *bb = (FeBasicBlock){0};
}