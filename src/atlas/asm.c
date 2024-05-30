#include "atlas.h"

AsmSymbol* air_sym_to_asm_sym(AsmModule* m, AIR_Symbol* sym) {
    AsmSymbol* new_sym = arena_alloc(&m->alloca, sizeof(*new_sym), alignof(*new_sym));
    new_sym->binding = sym->visibility;
    new_sym->name = string_clone(sym->name);
    return new_sym;
}

AsmModule* asm_new_module(TargetInfo* target) {
    AsmModule* m = mars_alloc(sizeof(AsmModule));

    m->target = target;
    m->alloca = arena_make(0x1000);
    return m;
}

AsmFunction* asm_new_function(AsmModule* m, AsmSymbol* sym) {
    AsmFunction* f = mars_alloc(sizeof(*f));

    f->mod = m;

    f->sym = sym;
    f->sym->func = f; // set the backlink

    m->functions = mars_realloc(m->functions, sizeof(AsmFunction*) * (m->functions_len+1));
    assert(m->functions != NULL);
    m->functions[m->functions_len++] = f;

    da_init(&f->vregs, 16);

    return f;
}

AsmGlobal* asm_new_global(AsmModule* m, AsmSymbol* sym) {
    AsmGlobal* g = mars_alloc(sizeof(*g));

    g->sym = sym;
    g->sym->glob = g; // set backlink

    m->globals = mars_realloc(m->globals, sizeof(AsmGlobal*) * (m->globals_len+1));
    assert(m->globals != NULL);
    m->globals[m->globals_len++] = g;

    return g;
}

AsmBlock* asm_new_block(AsmFunction* f, string label) {
    AsmBlock* b = mars_alloc(sizeof(*b));

    da_init(b, 10);
    b->label = label;
    b->f = f;

    return b;
}

AsmInst* asm_add_inst(AsmBlock* b, AsmInst* inst) {
    da_append(b, inst);
    return inst;
}

AsmInst* asm_new_inst(AsmModule* m, u32 template) {
    AsmInst* i = arena_alloc(&m->alloca, sizeof(AsmInst), alignof(AsmInst));
    *i = (AsmInst){0};
    i->template = &m->target->insts[template];

    if (i->template->num_imms != 0) {
        i->imms = arena_alloc(&m->alloca, sizeof(i->imms[0]) * i->template->num_imms, alignof(i->imms[0]));
    }
    if (i->template->num_ins != 0) {
        i->ins = arena_alloc(&m->alloca, sizeof(i->ins[0]) * i->template->num_ins, alignof(i->ins[0]));
    }
    if (i->template->num_outs != 0) {
        i->outs = arena_alloc(&m->alloca, sizeof(i->outs[0]) * i->template->num_outs, alignof(i->outs[0]));
    }
    
    return i;
}

VReg* asm_new_vreg(AsmModule* m, AsmFunction* f, u32 regclass) {
    VReg* r = arena_alloc(&m->alloca, sizeof(*r), alignof(*r));
    
    r->required_regclass = regclass;
    r->real = ATLAS_PHYS_UNASSIGNED;


    da_append(&f->vregs, r);
    

    return r;
}