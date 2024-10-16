#include "iron/iron.h"
#include "passes/passes.h"

void fe_typegraph_init(FeModule* m);
FeModule* fe_new_module(string name) {
    FeModule* mod = fe_malloc(sizeof(*mod));
    *mod = (FeModule){0};

    mod->name = name;

    da_init(&mod->symtab, 32);

    fe_typegraph_init(mod);

    da_init(&mod->pass_queue, 4);

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

    for_urange(i, 0, m->functions_len) {
        fe_destroy_function(m->functions[i]);
    }
    fe_free(m->functions);

    fe_free(m->datas);

    da_destroy(&m->pass_queue);
}

void fe_destroy_function(FeFunction* f) {
    fe_free(f->params.at);
    fe_free(f->returns.at);
    da_destroy(&f->stack);
    foreach (FeBasicBlock* bb, f->blocks) {
        fe_destroy_basic_block(bb);
    }
    da_destroy(&f->blocks);
    arena_delete(&f->alloca);
    *f = (FeFunction){0};
}

void fe_destroy_basic_block(FeBasicBlock* bb) {
    *bb = (FeBasicBlock){0};
}

static FeAllocator fe_global_allocator = {
    .malloc = malloc,
    .realloc = realloc,
    .free = free,
};

void fe_set_allocator(FeAllocator alloc) {
    fe_global_allocator = alloc;
}

void* fe_malloc(size_t size) {
    return memset(fe_global_allocator.malloc(size), 0, size);
}

void* fe_realloc(void* ptr, size_t size) {
    return fe_global_allocator.realloc(ptr, size);
}

void fe_free(void* ptr) {
    fe_global_allocator.free(ptr);
}