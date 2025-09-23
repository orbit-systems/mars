#include "common/util.h"

#include "iron/iron.h"
#include <string.h>

static const char* inst_name(FeInst* inst) {
    FeInst* bookend = inst;
    while (bookend->kind != FE__BOOKEND) {
        bookend = bookend->next;
    }
    const FeTarget* t = fe_extra(bookend, FeInst_Bookend)->block->func->mod->target;
    return fe_inst_name(t, inst->kind);
}

FeModule* fe_module_new(FeArch arch, FeSystem system) {
    FeModule* mod = fe_malloc(sizeof(*mod));
    memset(mod, 0, sizeof(*mod));

    mod->target = fe_make_target(arch, system);
    fe_symtab_init(&mod->symtab);
    return mod;
}

void fe_module_destroy(FeModule* mod) {
    // destroy the functions
    while (mod->funcs.first) {
        fe_func_destroy(mod->funcs.first);  
    }

    fe_free((void*)mod->target);
    fe_free(mod);
}

// void fe_section_destroy(FeSection* section) {

// }

FeSection* fe_section_new(FeModule* m, const char* name, u16 len, FeSectionFlags flags) {
    FeSection* section = fe_malloc(sizeof(*section));
    memset(section, 0, sizeof(*section));

    if (len == 0) {
        len = strlen(name);
    }

    section->name = fe_compstr(name, len);
    section->flags = flags;

    if (m->sections.first == nullptr) {
        m->sections.first = section;
        m->sections.last = section;
    } else {
        section->prev = m->sections.last;
        m->sections.last->next = section;
        m->sections.last = section;
    }

    return section;
}

FeComplexTy* fe_ty_record_new(u32 fields_len) {
    FeComplexTy* record = fe_malloc(sizeof(*record));
    record->ty = FE_TY_RECORD;
    record->record.fields_len = fields_len;
    record->record.fields = fe_malloc(sizeof(FeRecordField) * fields_len);

    return record;
}

// if elem_ty is not a complex type, elem_cty should be nullptr
FeComplexTy* fe_ty_array_new(u32 array_len, FeTy elem_ty, FeComplexTy* elem_cty) {
    FeComplexTy* array = fe_malloc(sizeof(*array));
    array->ty = FE_TY_ARRAY;
    array->array.elem_ty = elem_ty;
    array->array.complex_elem_ty = elem_cty;
    array->array.len = array_len;
    return array;
}

usize fe_ty_get_align(FeTy ty, FeComplexTy *cty) {
    switch (ty) {
    case FE_TY_VOID:
        FE_CRASH("cant get align of void");
    case FE_TY_TUPLE:
        FE_CRASH("cant get align of tuple");
    case FE_TY_ARRAY:
        if (cty != nullptr) {
            FE_CRASH("ty is array but no FeComplexTy was provided");
        }
        return fe_ty_get_align(cty->array.elem_ty, cty->array.complex_elem_ty);
    case FE_TY_RECORD: {
        if (cty != nullptr) {
            FE_CRASH("ty is record but no FeComplexTy was provided");
        }
        usize align = 1;
        FeRecordField* fields = cty->record.fields;
        for_n(i, 0, cty->record.fields_len) {
            usize field_align = fe_ty_get_align(fields[i].ty, fields[i].complex_ty);
            if (align < field_align) {
                align = field_align;
            }
        }
        return align;
    }
    case FE_TY_BOOL:
    case FE_TY_I8:
        return 1;
    case FE_TY_I16:
        return 2;
    case FE_TY_I32:
    case FE_TY_F32:
        return 4;
    case FE_TY_I64:
    case FE_TY_F64:
        return 8;
    default:
        if (FE_TY_IS_VEC(ty)) {
            switch (FE_TY_VEC_SIZETY(ty)) {
            case FE_TY_V128:
                return 16;
            case FE_TY_V256:
                return 32;
            case FE_TY_V512:
                return 64;
            default:
                FE_CRASH("insane vector type");
            }
        }
        FE_CRASH("unknown ty %u", ty);
    }
}

usize fe_ty_get_size(FeTy ty, FeComplexTy *cty) {
    switch (ty) {
    case FE_TY_VOID:
        FE_CRASH("cant get size of void");
    case FE_TY_TUPLE:
        FE_CRASH("cant get size of tuple");
    case FE_TY_ARRAY:
        if (cty == nullptr) {
            FE_CRASH("ty is array but no FeComplexTy was provided");
        }
        return cty->array.len * fe_ty_get_size(cty->array.elem_ty, cty->array.complex_elem_ty);
    case FE_TY_RECORD: {
        if (cty == nullptr) {
            FE_CRASH("ty is record but no FeComplexTy was provided");
        }

        usize align = fe_ty_get_align(ty, cty);

        FeRecordField* last_field = &cty->record.fields[cty->record.fields_len];
        return last_field->offset + last_field->offset;
    }
    case FE_TY_BOOL:
    case FE_TY_I8:
        return 1;
    case FE_TY_I16:
        return 2;
    case FE_TY_I32:
    case FE_TY_F32:
        return 4;
    case FE_TY_I64:
    case FE_TY_F64:
        return 8;
    default:
        if (FE_TY_IS_VEC(ty)) {
            switch (FE_TY_VEC_SIZETY(ty)) {
            case FE_TY_V128:
                return 16;
            case FE_TY_V256:
                return 32;
            case FE_TY_V512:
                return 64;
            default:
                FE_CRASH("insane vector type");
            }
        }
        FE_CRASH("unknown ty %u", ty);
    }
}

// if len == 0, calculate with strlen
FeSymbol* fe_symbol_new(FeModule* m, const char* name, u16 len, FeSection* section, FeSymbolBinding bind) {
    if (name == nullptr) {
        FE_CRASH("symbol cannot be null");
    }
    if (len == 0) {
        len = strlen(name);
    }
    if (len == 0) {
        FE_CRASH("symbol cannot have zero length");
    }
    // put this in an arena of some sort later
    FeSymbol* sym = fe_malloc(sizeof(FeSymbol));
    sym->kind = FE_SYMKIND_NONE;
    sym->bind = bind;
    sym->section = section;
    sym->name = fe_compstr(name, len);

    if (fe_symtab_get(&m->symtab, name, len)) {
        FE_CRASH("symbol with name '%.*s' already exists", len, name);
    }
    fe_symtab_put(&m->symtab, sym);

    return sym;
}

void fe_symbol_destroy(FeSymbol* sym) {
    memset(sym, 0, sizeof(*sym));
    fe_free(sym);
}

FeFuncSig* fe_funcsig_new(FeCallConv cconv, u16 param_len, u16 return_len) {
    FeFuncSig* sig = fe_malloc(
        sizeof(*sig) + (param_len + return_len) * sizeof(sig->params[0])
    );
    sig->cconv = cconv,
    sig->param_len = param_len;
    sig->return_len = return_len;
    return sig;
};

FeFuncParam* fe_funcsig_param(FeFuncSig* sig, u16 index) {
    if (index >= sig->param_len) {
        FE_CRASH("index > param_len");
    }
    return &sig->params[index];
}

FeFuncParam* fe_funcsig_return(FeFuncSig* sig, u16 index) {
    if (index >= sig->return_len) {
        FE_CRASH("index > return_len");
    }
    return &sig->params[sig->param_len + index];
}

void fe_funcsig_destroy(FeFuncSig* sig) {
    fe_free(sig);
}

FeBlock* fe_block_new(FeFunc* f) {
    FeBlock* block = fe_malloc(sizeof(*block));
    memset(block, 0, sizeof(*block));

    block->id = f->max_block_id++;

    // init predecessor list
    block->pred_len = 0;
    block->pred_cap = 2;
    block->pred = fe_ipool_list_alloc(f->ipool, block->pred_cap);

    // init successor list
    block->succ_len = 0;
    block->succ_cap = 2;
    block->succ = fe_ipool_list_alloc(f->ipool, block->succ_cap);

    block->func = f;
    
    // adds initial bookend instruction to block
    FeInst* bookend = fe_ipool_alloc(f->ipool, sizeof(FeInst_Bookend));
    bookend->kind = FE__BOOKEND;
    bookend->ty = FE_TY_VOID;
    bookend->next = bookend;
    bookend->prev = bookend;
    fe_extra(bookend, FeInst_Bookend)->block = block;
    block->bookend = bookend;

    // append to block list
    if (f->last_block) {
        FeBlock* last = f->last_block;
        block->list_prev = last;
        last->list_next = block;
        f->last_block = block;
    } else {
        f->last_block = block;
        f->entry_block = block;
    }
    return block;
}

void fe_block_destroy(FeBlock *block) {
    FeFunc* f = block->func;

    // remove from linked list
    if (block->list_next) {
        block->list_next->list_prev = block->list_prev;
    } else {
        f->last_block = block->list_prev;
    }
    if (block->list_prev) {
        block->list_prev->list_next = block->list_next;
    } else {
        f->entry_block = block->list_next;
    }

    for_inst(inst, block) {
        fe_inst_destroy(f, inst);
    }
    fe_inst_destroy(f, block->bookend);
    
    fe_ipool_list_free(f->ipool, block->pred, block->pred_cap);
    fe_ipool_list_free(f->ipool, block->succ, block->succ_cap);
    
    fe_free(block);
}

FeInstChain fe_chain_from_block(FeBlock* block) {
    FeInstChain chain;
    
    // extract it from the block's inst list
    chain.begin = block->bookend->next;
    chain.end = block->bookend->prev;
    chain.begin->prev = nullptr;
    chain.end->next = nullptr;
    
    // remove it from the block
    block->bookend->next = block->bookend;
    block->bookend->prev = block->bookend;
    return chain;
}

void fe_cfg_add_edge(FeFunc* f, FeBlock* pred, FeBlock* succ) {
    FeInstPool* pool = f->ipool;

    // append succ to pred's succ list
    if_unlikely (pred->succ_cap == pred->succ_len) {
        pred->succ_cap *= 2;
        FeBlock** new_list = fe_ipool_list_alloc(pool, pred->succ_len);
        memcpy(new_list, pred->succ, sizeof(pred->succ[0]) * pred->succ_len);
        fe_ipool_list_free(pool, pred->succ, pred->succ_len);
        pred->succ = new_list;
    }
    pred->succ[pred->succ_len] = succ;
    pred->succ_len += 1;

    // append pred to succ's pred list
    if_unlikely (succ->pred_cap == succ->pred_len) {
        succ->pred_cap *= 2;
        FeBlock** new_list = fe_ipool_list_alloc(pool, succ->pred_len);
        memcpy(new_list, succ->pred, sizeof(succ->pred[0]) * succ->pred_len);
        fe_ipool_list_free(pool, pred->pred, succ->pred_len);
        succ->pred = new_list;
    }
    succ->pred[succ->pred_len] = pred;
    succ->pred_len += 1;
}

void fe_cfg_remove_edge(FeBlock* pred, FeBlock* succ) {
    // find succ in pred's succ list
    for_n(i, 0, pred->succ_len) {
        if (pred->succ[i] == succ) {
            // unordered remove
            pred->succ[i] = pred->succ[--pred->succ_len];
            break;
        }
    }

    // find pred in succ's pred list
    for_n(i, 0, succ->pred_len) {
        if (succ->pred[i] == pred) {
            // unordered remove
            succ->pred[i] = succ->pred[--succ->pred_len];
            break;
        } 
    }
}

static inline usize usize_next_pow_2(usize x) {
    return 1 << ((sizeof(x) * 8) -_Generic(x,
        unsigned long long: __builtin_clzll(x - 1),
        unsigned long: __builtin_clzl(x - 1),
        unsigned int: __builtin_clz(x - 1)));
}

static inline usize usize_log2(usize x) {
    return (sizeof(x) * 8 - 1) - _Generic(x,
        unsigned long long: __builtin_clzll(x),
        unsigned long: __builtin_clzl(x),
        unsigned int: __builtin_clz(x));
}

static usize count_composite_items(FeTy ty, FeComplexTy* cty) {
    if (ty == FE_TY_ARRAY) {
        return cty->array.len * count_composite_items(
            cty->array.elem_ty, 
            cty->array.complex_elem_ty
        );
    } else if (ty == FE_TY_RECORD) {
        usize n = 0;
        for_n(i, 0, cty->record.fields_len) {
            FeRecordField* field = &cty->record.fields[i];
            n += count_composite_items(field->ty, field->complex_ty);
        }
        return n;
    }
    return 1;
}

static void place_params(FeFunc* f, FeInst* root, FeTy ty, FeComplexTy* cty, usize* index) {
    if (ty == FE_TY_ARRAY) {
        for_n (i, 0, cty->array.len) {
            place_params(
                f,
                root,
                cty->array.elem_ty, 
                cty->array.complex_elem_ty,
                index
            );
        }
    } else if (ty == FE_TY_RECORD) {
        for_n(i, 0, cty->record.fields_len) {
            FeRecordField* field = &cty->record.fields[i];
            place_params(f, root, field->ty, field->complex_ty, index);
        }
    } else {
        FeInst* param = fe_inst_new(f, 1, sizeof(FeInstProj));
        param->kind = FE_PROJ;
        param->ty = ty;

        fe_extra(param, FeInstProj)->index = *index;
        f->params[*index] = param;

        fe_set_input(f, param, 0, root);

        fe_append_end(f->entry_block, param);
        
        *index += 1;
    }
}

FeFunc* fe_func_new(FeModule* mod, FeSymbol* sym, FeFuncSig* sig, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeFunc* f = fe_malloc(sizeof(FeFunc));
    memset(f, 0, sizeof(*f));
    f->sig = sig;
    f->mod = mod;
    f->ipool = ipool;
    f->vregs = vregs;
    f->sym = sym;
    sym->func = f;
    sym->kind = FE_SYMKIND_FUNC;

    // append to function list
    if (mod->funcs.first == nullptr) {
        mod->funcs.first = f;
        mod->funcs.last = f;
    } else {
        f->list_prev = mod->funcs.last;
        f->list_prev->list_next = f;
        mod->funcs.last = f;
    }
    
    usize num_params = 0;
    for_n(i, 0, sig->param_len) {
        FeFuncParam* p = &sig->params[i];
        num_params += count_composite_items(p->ty, p->cty);
    }

    // add initial basic block
    f->entry_block = f->last_block = fe_block_new(f);

    f->params = fe_malloc(sizeof(f->params[0]) * num_params);

    // add root node
    FeInst* root = fe_inst_new(f, 0, 0);
    root->kind = FE__ROOT;
    root->ty = FE_TY_VOID;

    fe_append_begin(f->entry_block, root);

    // adds parameter instructions
    usize index = 0;
    for_n(i, 0, sig->param_len) {
        place_params(
            f, root, 
            sig->params[i].ty, 
            sig->params[i].cty, 
            &index
        );
    }

    return f;
}

void fe_func_destroy(FeFunc *f) {
    if (f->params) {
        fe_free(f->params);    
    }

    // free the block list
    while (f->entry_block) {
        fe_block_destroy(f->entry_block);
    }
    
    // free the stack
    while (f->stack_top) {
        fe_free(fe_stack_remove(f, f->stack_top));
    }

    // remove from linked list
    if (f->list_next == nullptr) { // at back of list
        f->mod->funcs.last = f->list_prev;
    } else {
        f->list_next->list_prev = f->list_prev;
    }
    if (f->list_prev == nullptr) { // at front of list
        f->mod->funcs.first = f->list_next;
    } else {
        f->list_prev->list_next = f->list_next;
    }
    // f->list_next = NULL;
    // f->list_prev = NULL;
    fe_free(f);
}

FeInst* fe_func_param(FeFunc* f, u16 index) {
    return f->params[index];
}

FeBlock** fe_list_terminator_successors(const FeTarget* t, FeInst* term, usize* len_out) {
    if (!fe_inst_has_trait(term->kind, FE_TRAIT_TERMINATOR)) {
        FE_CRASH("list_targets: inst %s is not a terminator", inst_name(term));
    }
    if (term->kind > FE__BASE_INST_END) {
        return t->list_targets(term, len_out);
    }

    switch (term->kind) {
    case FE_JUMP:
        *len_out = 1;
        return &fe_extra(term, FeInstJump)->to;
    case FE_BRANCH:
        *len_out = 2;
        return &fe_extra(term, FeInstBranch)->if_true;
    case FE_RETURN:
    default:
        *len_out = 0;
        return nullptr;
    }
}

// remove inst from its basic block
FeInst* fe_inst_remove_from_block(FeInst* inst) {
    if (inst->next) {
        inst->next->prev = inst->prev;
    }
    if (inst->prev) {
        inst->prev->next = inst->next;
    }
    inst->next = nullptr;
    inst->prev = nullptr;
    return inst;
}

// insert 'i' before 'point' in a basic block
FeInst* fe_insert_before(FeInst* point, FeInst* i) {
    FeInst* p_prev = point->prev;
    p_prev->next = i;
    point->prev = i;
    i->next = point;
    i->prev = p_prev;
    return i;
}

// insert 'i' after 'point' in a basic block
FeInst* fe_insert_after(FeInst* point, FeInst* i) {
    FeInst* p_next = point->next;
    p_next->prev = i;
    point->next = i;
    i->prev = point;
    i->next = p_next;
    return i;
}

// replace 'from' with 'to' in a basic block
void fe_inst_replace_pos(FeInst* from, FeInst* to) {
    from->next->prev = to;
    from->prev->next = to;
    to->next = from->next;
    to->prev = from->prev;
}

FeInstChain fe_chain_new(FeInst* initial) {
    FeInstChain chain = {0};
    chain.begin = initial;
    chain.end = initial;
    return chain;
}

FeInstChain fe_chain_append_end(FeInstChain chain, FeInst* i) {
    chain.end->next = i;
    i->prev = chain.end;
    chain.end = i;
    return chain;
}

FeInstChain fe_chain_append_begin(FeInstChain chain, FeInst* i) {
    chain.begin->prev = i;
    i->next = chain.begin;
    chain.begin = i;
    return chain;
}

FeInstChain fe_chain_concat(FeInstChain front, FeInstChain back) {
    if (front.begin == nullptr) {
        return back;
    }
    if (back.begin == nullptr) {
        return front;
    }

    front.end->next = back.begin;
    back.begin->prev = front.end;
    front.end = back.end;

    return front;
}
void fe_insert_chain_before(FeInst* point, FeInstChain chain) {
    FeInst* p_prev = point->prev;
    p_prev->next = chain.begin;
    point->prev = chain.end;
    chain.end->next = point;
    chain.begin->prev = p_prev;
}

void fe_insert_chain_after(FeInst* point, FeInstChain chain) {
    FeInst* p_next = point->next;
    p_next->prev = chain.end;
    point->next = chain.begin;
    chain.begin->prev = point;
    chain.end->next = p_next;
}

void fe_chain_replace_pos(FeInst* from, FeInstChain to) {
    from->next->prev = to.end;
    from->prev->next = to.begin;
    to.end->next = from->next;
    to.begin->prev = from->prev;
    from->next = nullptr;
    from->prev = nullptr;
}

void fe_chain_destroy(FeFunc* f, FeInstChain chain) {
    for (FeInst* inst = chain.begin, *next = inst->next; inst == nullptr; inst = next, next = next->next) {
        fe_inst_destroy(f, inst);
    }
}

// -------------------------------------
// instruction builders
// -------------------------------------

void fe_set_input(FeFunc* f, FeInst* inst, u16 n, FeInst* input) {
    FE_ASSERT(n < inst->in_len);
    FE_ASSERT(input != nullptr); // maybe fix it to handle this case?
    
    FeInst* old_input = inst->inputs[n];

    if (old_input != nullptr) {
        // unordered remove inst from old_input->uses
        usize use_index = USIZE_MAX;
        for_n (i, 0, old_input->use_len) {
            FeInstUse old_input_use = old_input->uses[i];
            if (old_input_use.idx == n && FE_USE_PTR(old_input_use) == inst) {
                use_index = i;
                break;
            }
        }
        FE_ASSERT(use_index != USIZE_MAX);

        old_input->use_len -= 1;
        old_input->uses[use_index] = old_input->uses[old_input->use_len];
    }

    // add inst to input's uses
    inst->inputs[n] = input;
    if_unlikely (input->use_cap == input->use_len) {
        FeInstPool* pool = f->ipool;
        
        input->use_cap *= 2;
        // copy uses to new larger list
        FeInstUse* new_uses = fe_ipool_list_alloc(pool, input->use_cap);
        memcpy(new_uses, input->uses, sizeof(new_uses[0]) * input->use_len);
       
        // set the top list to zero
        memset(&new_uses[input->use_len], 0, sizeof(new_uses[0]) * input->use_len);
        
        // free old list
        fe_ipool_list_free(pool, input->uses, input->use_len);

        input->uses = new_uses;
    }

    input->uses[input->use_len].idx = n;
    input->uses[input->use_len].ptr = (i64)inst;
    input->use_len += 1;
}

void fe_set_input_null(FeInst* inst, u16 n) {
    FE_ASSERT(n < inst->in_len);
    FeInst* old_input = inst->inputs[n];

    if (old_input != nullptr) {
        // unordered remove inst from old_input->uses
        usize use_index = USIZE_MAX;
        for_n (i, 0, old_input->use_len) {
            FeInstUse old_input_use = old_input->uses[i];
            if (old_input_use.idx == n && FE_USE_PTR(old_input_use) == inst) {
                use_index = i;
                break;
            }
        }
        FE_ASSERT(use_index != USIZE_MAX);

        old_input->use_len -= 1;
        old_input->uses[use_index] = old_input->uses[old_input->use_len];

        // set current input to null
        inst->inputs[n] = nullptr;
    }
}

// replace users of 'old_val' with users of 'new_val'
usize fe_replace_uses(FeFunc* f, FeInst* old_val, FeInst* new_val) {
    // kinda unsafe, directly manipulating inputs and use lists is not great
    for_n (i, 0, old_val->use_len) {
        FeInstUse old_val_use = old_val->uses[i];
        FeInst* old_val_use_inst = FE_USE_PTR(old_val_use);
        u16 old_val_use_input = old_val_use.idx; // which input of 'old_val_use_inst' is 'old_val'

        // unlink so fe_set_input doesnt try to touch our precious use list
        old_val_use_inst->inputs[old_val_use_input] = nullptr;

        fe_set_input(f, old_val_use_inst, old_val_use_input, new_val);
    }

    usize num_replaced = old_val->use_len;
    old_val->use_len = 0;
    
    return num_replaced;
}

// #include <stdio.h>

FeInst* fe_inst_new(FeFunc* f, usize input_len, usize extra_size) {
    FeInst* inst = fe_ipool_alloc(f->ipool, extra_size);
    inst->in_len = input_len;
    inst->id = f->max_id++;

    if (input_len != 0) {
        inst->in_cap = usize_next_pow_2(input_len);
        inst->inputs = fe_ipool_list_alloc(f->ipool, inst->in_cap);
        memset(inst->inputs, 0, sizeof(inst->inputs[0]) * inst->in_cap);
    } else {
        inst->in_cap = 0;
        inst->inputs = nullptr;
    }

    inst->use_len = 0;
    inst->use_cap = 2;
    inst->uses = fe_ipool_list_alloc(f->ipool, inst->use_cap);
    memset(inst->uses, 0, sizeof(inst->uses[0]) * inst->use_cap);

    return inst;
}

void fe_inst_add_input(FeFunc* f, FeInst* inst, FeInst* input) {

    // expand dong
    if_unlikely (inst->in_len == inst->in_cap) {
        FeInstPool* pool = f->ipool;

        inst->in_cap *= 2;
        // copy inputs to new larger list
        FeInst** new_inputs = fe_ipool_list_alloc(pool, inst->in_cap);
        memcpy(new_inputs, inst->inputs, sizeof(new_inputs[0]) * inst->in_len);

        // set the top list to zero
        memset(&new_inputs[input->in_len], 0, sizeof(new_inputs[0]) * input->in_len);

        fe_ipool_list_free(pool, input->inputs, input->in_len);
        inst->inputs = new_inputs;
    }

    inst->inputs[inst->in_len] = input;
    inst->in_len++;
}

void fe_inst_destroy(FeFunc* f, FeInst* inst) {

    FE_ASSERT(inst);
    FE_ASSERT(inst->use_len == 0);

    // disconnect from other nodes
    for_n(i, 0, inst->in_len) {
        fe_set_input_null(inst, i);
    }

    // remove from block
    fe_inst_remove_from_block(inst);

    // free inputs and uses lists
    if (inst->inputs != nullptr) {
        fe_ipool_list_free(f->ipool, inst->inputs, inst->in_cap);
    }
    fe_ipool_list_free(f->ipool, inst->uses, inst->use_cap);

    // free instruction itself
    fe_ipool_free(f->ipool, inst);
}


FeInst* fe_inst_proj(FeFunc* f, FeInst* inst, usize index) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstProj));
    i->kind = FE_PROJ;
    i->ty = fe_proj_ty(inst, index);

    fe_extra(i, FeInstProj)->index = index;

    return i;
}

FeInst* fe_inst_const(FeFunc* f, FeTy ty, u64 val) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstConst));
    i->kind = FE_CONST;
    i->ty = ty;

    fe_extra(i, FeInstConst)->val = val;

    return i;
}

FeInst* fe_inst_const_f64(FeFunc* f, f64 val) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstConst));
    i->kind = FE_CONST;
    i->ty = FE_TY_F64;

    fe_extra(i, FeInstConst)->val_f64 = val;

    return i;
}

FeInst* fe_inst_const_f32(FeFunc* f, f32 val) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstConst));
    i->kind = FE_CONST;
    i->ty = FE_TY_F32;

    fe_extra(i, FeInstConst)->val_f32 = val;

    return i;
}

FeInst* fe_inst_stack_addr(FeFunc* f, FeStackItem* item) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstStack));
    i->kind = FE_STACK_ADDR;
    i->ty = f->mod->target->ptr_ty;

    fe_extra(i, FeInstStack)->item = item;

    return i;
}

FeInst* fe_inst_sym_addr(FeFunc* f, FeSymbol* sym) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstSymAddr));
    i->kind = FE_SYM_ADDR;
    i->ty = f->mod->target->ptr_ty;

    fe_extra(i, FeInstSymAddr)->sym = sym;

    return i;
}

FeInst* fe_inst_unop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* val) {
    FeInst* i = fe_inst_new(f, 1, sizeof(FeInstSymAddr));
    i->kind = kind;
    i->ty = ty;

    fe_set_input(f, i, 0, 
        val);

    return i;
}

FeInst* fe_inst_binop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* lhs, FeInst* rhs) {
    FeInst* i = fe_inst_new(f, 2, sizeof(FeInstSymAddr));
    i->kind = kind;
    i->ty = ty;

    fe_set_input(f, i, 0, lhs);
    fe_set_input(f, i, 1, rhs);

    return i;
}

FeInst* fe_inst_load(FeFunc* f, FeTy ty, FeInst* ptr, u16 align, u8 offset) {
    FeInst* i = fe_inst_new(f, 2, sizeof(FeInstMemop));
    i->kind = FE_LOAD;
    i->ty = ty;

    if (align == FE_MEMOP_ALIGN_DEFAULT) {
        align = fe_ty_get_align(ty, nullptr);
    }

    fe_set_input_null(i, 0); // no last_effect yet
    fe_set_input(f, i, 1, ptr); // pointer

    fe_extra(i, FeInstMemop)->align = align;
    fe_extra(i, FeInstMemop)->offset = offset;
    fe_extra(i, FeInstMemop)->alias_space = 0; // everything in the same alias space atm

    return i;
}

FeInst* fe_inst_store(FeFunc* f, FeInst* ptr, FeInst* val, u16 align, u8 offset) {
    FeInst* i = fe_inst_new(f, 3, sizeof(FeInstMemop));
    i->kind = FE_STORE;
    i->ty = FE_TY_VOID;

    if (align == FE_MEMOP_ALIGN_DEFAULT) {
        align = fe_ty_get_align(val->ty, nullptr);
    }

    fe_set_input_null(i, 0); // no last_effect yet
    fe_set_input(f, i, 1, ptr); // pointer 
    fe_set_input(f, i, 2, val); // value to store

    fe_extra(i, FeInstMemop)->align = align;
    fe_extra(i, FeInstMemop)->offset = offset;
    fe_extra(i, FeInstMemop)->alias_space = 0; // everything in the same alias space atm

    return i;
}

FeInst* fe_inst_call(FeFunc* f, FeInst* callee, FeFuncSig* sig) {
    FeInst* i = fe_inst_new(f, 2 + sig->param_len, sizeof(FeInstMemop));
    i->kind = FE_STORE;
    if (sig->param_len == 0) {
        i->ty = FE_TY_VOID;
    } else {
        i->ty = FE_TY_TUPLE;
    }

    fe_set_input_null(i, 0); // no last_effect yet
    fe_set_input(f, i, 0, callee);

    for_n (n, 0, sig->param_len) {
        // fill in parameters
        fe_set_input_null(i, n + 2);
    }

    return i;
}

void fe_call_set_arg(FeFunc* f, FeInst* call, u16 n, FeInst* arg) {
    fe_set_input(f, call, n + 2, arg);
}

FeInst* fe_inst_return(FeFunc* f) {
    FeInst* i = fe_inst_new(f, 1 + f->sig->return_len, sizeof(FeInstMemop));
    i->kind = FE_RETURN;
    i->ty = FE_TY_VOID;

    fe_set_input_null(i, 0); // no last_effect yet

    for_n (n, 0, f->sig->return_len) {
        // fill in return args
        fe_set_input_null(i, n + 1);
    }

    return i;
}

void fe_return_set_arg(FeFunc* f, FeInst* ret, u16 n, FeInst* arg) {
    fe_set_input(f, ret, n + 1, arg);
}

FeInst* fe_inst_phi(FeFunc* f, FeTy ty, u16 expected_len) {
    FeInst* i = fe_inst_new(f, expected_len, sizeof(FeInstPhi));
    i->kind = FE_PHI;
    i->ty = ty;
    i->in_len = 0;

    fe_extra(i, FeInstPhi)->blocks = fe_ipool_list_alloc(f->ipool, i->in_cap);

    return i;
}

FeInst* fe_inst_mem_phi(FeFunc* f, u16 expected_len) {
    FeInst* i = fe_inst_new(f, expected_len, sizeof(FeInstPhi));
    i->kind = FE_MEM_PHI;
    i->ty = FE_TY_VOID;
    i->in_len = 0;

    fe_extra(i, FeInstPhi)->blocks = fe_ipool_list_alloc(f->ipool, i->in_cap);

    return i;
}

void fe_phi_add_src(FeFunc* f, FeInst* phi, FeInst* src_value, FeBlock* src_block) {

    FeInstPhi* phi_data = fe_extra(phi);

    if_unlikely (phi->in_len == phi->in_cap) {
        FeInstPool* pool = f->ipool;

        usize new_cap = phi->in_cap * 2;
        // copy blocks to new larger list
        FeBlock** new_blocks = fe_ipool_list_alloc(pool, new_cap);
        memcpy(new_blocks, phi_data->blocks, sizeof(new_blocks[0]) * phi->in_len);

        // set the top list to zero
        memset(&new_blocks[phi->in_len], 0, sizeof(new_blocks[0]) * phi->in_len);

        fe_ipool_list_free(pool, phi_data->blocks, phi->in_len);
        phi_data->blocks = new_blocks;
    }
    
    phi_data->blocks[phi->in_len] = src_block;

    fe_inst_add_input(f, phi, src_value);
}

void fe_phi_remove_src(FeFunc* f, FeInst* phi, u16 n) {
    FeInstPhi* phi_data = fe_extra(phi);
    
    phi->in_len -= 1;
    phi_data->blocks[n] = phi_data->blocks[phi->in_len]; 
    phi->inputs[n] = phi->inputs[phi->in_len];
}

FeInst* fe_inst_branch(FeFunc* f, FeInst* cond) {
    FeInst* i = fe_inst_new(f, 1, sizeof(FeInstBranch));
    i->kind = FE_BRANCH;
    i->ty = FE_TY_VOID;

    FE_ASSERT(cond->ty == f->mod->target->ptr_ty);
    fe_set_input(f, i, 0, cond);

    return i;
}

void fe_branch_set_true(FeFunc* f, FeInst* branch, FeBlock* block) {
    FeInst* bookend = branch->next;
    FE_ASSERT(bookend && "branch is not in a block");
    FE_ASSERT(bookend->kind == FE__BOOKEND && "branch is not at the end of a block");

    FeBlock* pred = fe_extra(bookend, FeInst_Bookend)->block;
    FeInstBranch* branch_data = fe_extra(branch);

    if (branch_data->if_true) {
        fe_cfg_remove_edge(pred, branch_data->if_true);
    }

    fe_cfg_add_edge(f, pred, block);

    branch_data->if_true = block;
}

void fe_branch_set_false(FeFunc* f, FeInst* branch, FeBlock* block) {
    FeInst* bookend = branch->next;
    FE_ASSERT(bookend && "branch is not in a block");
    FE_ASSERT(bookend->kind == FE__BOOKEND && "branch is not at the end of a block");

    FeBlock* pred = fe_extra(bookend, FeInst_Bookend)->block;
    FeInstBranch* branch_data = fe_extra(branch);

    if (branch_data->if_false) {
        fe_cfg_remove_edge(pred, branch_data->if_false);
    }

    fe_cfg_add_edge(f, pred, block);

    branch_data->if_false = block;
}

FeInst* fe_inst_jump(FeFunc* f) {
    FeInst* i = fe_inst_new(f, 0, sizeof(FeInstBranch));
    i->kind = FE_BRANCH;
    i->ty = FE_TY_VOID;

    return i;
}

void fe_jump_set_target(FeFunc* f, FeInst* jump, FeBlock* block) {
    FeInst* bookend = jump->next;
    FE_ASSERT(bookend && "jump is not in a block");
    FE_ASSERT(bookend->kind == FE__BOOKEND && "jump is not at the end of a block");

    FeBlock* pred = fe_extra(bookend, FeInst_Bookend)->block;
    fe_cfg_add_edge(f, pred, block);

    fe_extra(jump, FeInstJump)->to = block;
}

// -------------------------------------
// instruction utlities
// -------------------------------------

FeTy fe_proj_ty(FeInst* tuple, usize index) {
    if (tuple->ty != FE_TY_TUPLE) {
        FE_CRASH("projection on non-tuple inst %s", inst_name(tuple));
    }
    switch (tuple->kind) {
    case FE_CALL:
        ;
        FeInstCall* icall = fe_extra(tuple);
        if (index < icall->sig->return_len) {
            return fe_funcsig_return(icall->sig, index)->ty;
        }
        FE_CRASH("index %zu out of bounds for [0, %u)", icall->sig->return_len);
    default:
        FE_CRASH("unknown inst kind (%s) %u", inst_name(tuple), tuple->kind);
    }
}

// -------------------------------------
// instruction traits
// -------------------------------------

#include "short_traits.h"

static FeTrait inst_traits[FE__INST_END] = {
    [FE__ROOT] = VOL | MEM_DEF,
    [FE_PROJ] = 0,
    [FE_CONST] = 0,
    [FE_STACK_ADDR] = 0,
    [FE_SYM_ADDR] = 0,

    [FE_IADD] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_ISUB] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_IMUL] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_IDIV] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_UDIV] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_IREM] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_UREM] = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_AND]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_OR]   = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_XOR]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | ASSOC | FAST_ASSOC,
    [FE_SHL]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_USR]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_ISR]  = BINOP | INT_IN | VEC_IN | SAME_IN_OUT,
    [FE_ILT]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_ULT]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_ILE]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_ULE]  = BINOP | INT_IN | SAME_INS | BOOL_OUT,
    [FE_IEQ]  = BINOP | INT_IN | VEC_IN | SAME_INS | BOOL_OUT | COMMU,

    [FE_FADD] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | FAST_ASSOC,
    [FE_FSUB] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_FMUL] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS | COMMU | FAST_ASSOC,
    [FE_FDIV] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,
    [FE_FREM] = BINOP | FLT_IN | VEC_IN | SAME_IN_OUT | SAME_INS,

    [FE_MOV] = UNOP | SAME_IN_OUT | MOV_HINT,
    [FE__MACH_MOV] = UNOP | VOL | SAME_IN_OUT | MOV_HINT,
    [FE__MACH_UPSILON]  = UNOP | VOL | SAME_IN_OUT | MOV_HINT,
    [FE_TRUNC] = UNOP | INT_IN,
    [FE_SIGN_EXT] = UNOP | INT_IN,
    [FE_ZERO_EXT] = UNOP | INT_IN,
    [FE_BITCAST] = UNOP,
    [FE_I2F] = UNOP | INT_IN,
    [FE_F2I] = UNOP | FLT_IN,
    [FE_U2F] = UNOP | INT_IN,
    [FE_F2U] = UNOP | FLT_IN,

    [FE_STORE] = VOL | MEM_USE | MEM_DEF,
    [FE_MEM_BARRIER] = VOL | MEM_USE | MEM_DEF,
    [FE_LOAD] = MEM_USE,
    [FE_CALL] = MEM_USE | MEM_DEF,

    [FE_UNREACHABLE] = TERM | VOL,
    [FE_BRANCH] = TERM | VOL,
    [FE_JUMP]   = TERM | VOL,
    [FE_RETURN] = TERM | VOL | MEM_USE,

    [FE__MACH_RETURN] = TERM | VOL,
};

FeTrait fe_inst_traits(FeInstKind kind) {
    if (kind > FE__INST_END) return 0;
    return inst_traits[kind];
}

bool fe_inst_has_trait(FeInstKind kind, FeTrait trait) {
    if (kind > FE__INST_END) return false;
    return (inst_traits[kind] & trait) != 0;
}

void fe__load_trait_table(usize start_index, const FeTrait* table, usize len) {
    memcpy(&inst_traits[start_index], table, sizeof(table[0]) * len);
}

#define USIZE_BITS (sizeof(usize) * 8)

void fe_iset_init(FeInstSet* iset) {
    *iset = (FeInstSet){};
    iset->id_start = UINT32_MAX;
}

bool fe_iset_contains(FeInstSet* iset, FeInst* inst) {
    u32 id = inst->id;
    u32 id_block = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    if_likely (iset->id_start <= id_block && id_block < iset->id_end) {
        usize exists_block = iset->exists[id_block - iset->id_start];
        return (exists_block & id_bit) != 0;
    }
    return false;
}


void fe_iset_push(FeInstSet* iset, FeInst* inst) {
    u32 id = inst->id;
    u32 id_block = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    // construct the initial state for the set centered around this inst
    if_unlikely (iset->id_start == UINT32_MAX) {
        iset->id_start = id_block;
        iset->id_end   = id_block + 1;
        iset->exists = fe_malloc(sizeof(usize));
        // memset(iset->exists, 0, sizeof(usize));
        iset->insts = fe_malloc(sizeof(iset->insts[0]) * USIZE_BITS);

        iset->exists[0] = id_bit; 
        iset->insts[id - iset->id_start * USIZE_BITS] = inst;

        return;
    }

    // printf("push %u %p\n", id, inst);

    if_likely (iset->id_start <= id_block && id_block < iset->id_end) {
        usize exists_block = iset->exists[id_block - iset->id_start];
        // printf("> %064lb\n", exists_block);
        if (!(exists_block & id_bit)) {
            exists_block |= id_bit;
            iset->exists[id_block - iset->id_start] = exists_block;
            iset->insts[id - iset->id_start * USIZE_BITS] = inst;
        }
        // printf("> %064lb\n\n", exists_block);
        return;
    }

    if (iset->id_end <= id_block) {
        // expand upwards
        usize new_end = id_block + 1;
        usize new_size = new_end - iset->id_start;
        // we can realloc
        iset->insts = fe_realloc(iset->insts, sizeof(FeInst*) * new_size * USIZE_BITS);
        iset->exists = fe_realloc(iset->insts, sizeof(FeInst*) * new_size);
        // have to memset the newly allocated '.exists' space
        for_n (i, iset->id_end, new_end) {
            iset->exists[i] = 0;
        }
        // do this more efficiently later idk
        iset->id_end = new_end;
        // fe_iset_push(iset, inst);
        iset->exists[id_block] = id_bit;
        iset->insts[id - iset->id_start * USIZE_BITS] = inst;
    } else {
        // expand downwards
        FE_CRASH("fuck! implement downwards set expansion");
    }

}

void fe_iset_remove(FeInstSet* iset, FeInst* inst) {
    u32 id = inst->id;
    u32 exists_block_index = id / USIZE_BITS;
    usize id_bit = (usize)(1) << (id % USIZE_BITS);

    if_likely (iset->id_start <= exists_block_index && exists_block_index < iset->id_end) {
        usize exists_block = iset->exists[exists_block_index - iset->id_start];
        if (exists_block & id_bit) {
            iset->exists[exists_block_index - iset->id_start] = exists_block ^ id_bit;
        }
    }
}

static inline usize count_trailing_zeros(usize n) {
#if USIZE_MAX == UINT64_MAX
    return __builtin_ctzll(n);
#else
    return __builtin_ctz(n);
#endif
}

FeInst* fe_iset_pop(FeInstSet* iset) {

    u32 blocks_len = iset->id_end - iset->id_start;
    for_n (exists_block_index, 0, blocks_len) {

        usize exists_block = iset->exists[exists_block_index];
        if (exists_block == 0) {
            // we can skip this block
            continue;
        }

        // pop the next instruction in the set
        usize set_bit = count_trailing_zeros(exists_block);
        // return the instruction at that position
        FeInst* inst = iset->insts[set_bit + exists_block_index * USIZE_BITS];

        // printf("pop %u %p\n", inst->id, inst);

        // remove it from its 'exists' block
        // printf("> %064lb\n", iset->exists[exists_block_index]);
        iset->exists[exists_block_index] ^= (usize)(1) << set_bit;
        // printf("> %064lb\n\n", iset->exists[exists_block_index]);

        return inst;
    }

    return nullptr;
}

void fe_iset_destroy(FeInstSet* iset) {
    fe_free(iset->insts);
    fe_free(iset->exists);
    *iset = (FeInstSet){0};
}
