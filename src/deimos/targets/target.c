#include "deimos.h"

#include "target.h"

AsmModule* asm_new_module(TargetInfo* target) {
    AsmModule* m = malloc(sizeof(AsmModule));
    *m = (AsmModule){0};

    m->target = target;
    m->alloca = arena_make(0x1000);
    return m;
}

AsmFunction* asm_new_function(AsmModule* m, AsmSymbol* sym) {
    AsmFunction* f = malloc(sizeof(*f));

    f->mod = m;

    f->sym = sym;
    f->sym->func = f; // set the backlink

    m->functions = realloc(m->functions, sizeof(AsmFunction*) * (m->functions_len+1));
    assert(m->functions != NULL);
    m->functions[m->functions_len++] = f;

    return f;
}

AsmGlobal* asm_new_global(AsmModule* m, AsmSymbol* sym) {
    AsmGlobal* g = malloc(sizeof(*g));

    g->sym = sym;
    g->sym->glob = g; // set backlink

    m->globals = realloc(m->globals, sizeof(AsmGlobal*) * (m->globals_len+1));
    assert(m->globals != NULL);
    m->globals[m->globals_len++] = g;

    return g;
}

AsmBlock* asm_new_block(AsmFunction* f, string label) {
    AsmBlock* b = malloc(sizeof(*b));

    da_init(b, 10);
    b->label = label;
    b->f = f;

    return b;
}

AsmInst* asm_add_inst(AsmBlock* b, AsmInst* inst) {
    da_append(b, inst);
    return inst;
}

AsmInst* asm_new_inst(AsmModule* m, TargetInstInfo* template) {
    AsmInst* i = arena_alloc(&m->alloca, sizeof(AsmInst), alignof(AsmInst));
    *i = (AsmInst){0};
    i->template = template;

    if (template->num_imms != 0) {
        i->imms = arena_alloc(&m->alloca, sizeof(i->imms[0]) * template->num_imms, alignof(i->imms[0]));
    }
    if (template->num_ins != 0) {
        i->imms = arena_alloc(&m->alloca, sizeof(i->ins[0]) * template->num_ins, alignof(i->ins[0]));
    }
    if (template->num_outs != 0) {
        i->imms = arena_alloc(&m->alloca, sizeof(i->outs[0]) * template->num_outs, alignof(i->outs[0]));
    }
    
    return i;
}

VReg* asm_new_vreg(AsmModule* m, u32 regclass) {
    VReg* r = arena_alloc(&m->alloca, sizeof(*r), alignof(*r));
    
    r->required_regclass = regclass;
    r->real = REAL_REG_UNASSIGNED;

    return r;
}