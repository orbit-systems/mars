#include "iron.h"

#include "iron/passes/passes.h"

// if (sym == NULL), create new symbol with no name
FeFunction* fe_new_function(FeModule* mod, FeSymbol* sym) {
    FeFunction* fn = mars_alloc(sizeof(FeFunction));

    fn->sym = sym ? sym : fe_new_symbol(mod, strprintf("symbol%zu", sym), FE_VIS_GLOBAL);
    fn->sym->is_function = true;
    fn->sym->function = fn;

    fn->alloca = arena_make(FE_FN_ALLOCA_BLOCK_SIZE);
    da_init(&fn->blocks, 1);
    da_init(&fn->stack, 1);
    fn->entry_idx = 0;
    fn->params = NULL;
    fn->returns = NULL;
    fn->mod = mod;

    mod->functions = mars_realloc(mod->functions, sizeof(*mod->functions) * (mod->functions_len+1));
    mod->functions[mod->functions_len++] = fn;
    return fn;
}

FeStackObject* fe_new_stackobject(FeFunction* f, FeType* t) {
    FeStackObject* obj = arena_alloc(&f->alloca, sizeof(*obj), alignof(*obj));
    obj->t = t;
    da_append(&f->stack, obj);
    return obj;
}

// takes multiple FeType*
void fe_set_func_params(FeFunction* f, u16 count, ...) {
    f->params_len = count;

    if (f->params) mars_free(f->params);

    f->params = mars_alloc(sizeof(*f->params) * count);
    if (!f->params) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        FeFunctionItem* item = mars_alloc(sizeof(FeFunctionItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->T = va_arg(args, FeType*);
            if (item->T == NULL) {
                no_set = true;
            }
        }
        
        f->params[i] = item;
    }
    va_end(args);
}

// takes multiple entity*
void fe_set_func_returns(FeFunction* f, u16 count, ...) {
    f->returns_len = count;

    if (f->returns) mars_free(f->returns);

    f->returns = mars_alloc(sizeof(*f->returns) * count);
    if (!f->params) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        FeFunctionItem* item = mars_alloc(sizeof(FeFunctionItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->T = va_arg(args, FeType*);
            if (item->T == NULL) {
                no_set = true;
            }
        }
        
        f->returns[i] = item;
    }
    va_end(args);
}

// if (sym == NULL), create new symbol with default name
FeGlobal* fe_new_global(FeModule* mod, FeSymbol* sym, bool read_only) {
    FeGlobal* gl = mars_alloc(sizeof(FeGlobal));

    gl->sym = sym ? sym : fe_new_symbol(mod, strprintf("symbol%zu", sym), FE_VIS_GLOBAL);
    gl->sym->is_function = false;
    gl->sym->global = gl;
    gl->read_only = read_only;
    gl->data = NULL;
    gl->data_len = 0;

    mod->globals = mars_realloc(mod->globals, sizeof(*mod->globals) * (mod->globals_len+1));
    mod->globals[mod->globals_len++] = gl;
    return gl;
}

void fe_set_global_data(FeGlobal* global, u8* data, u32 data_len, bool zeroed) {
    global->is_symbol_ref = false;
    global->data = data;
    global->data_len = data_len;
    global->zeroed = zeroed;
}

void fe_set_global_symref(FeGlobal* global, FeSymbol* symref) {
    global->is_symbol_ref = true;
    global->symref = symref;
}

// WARNING: does NOT check if a symbol already exists
// in most cases, use air_find_or_create_symbol
FeSymbol* fe_new_symbol(FeModule* mod, string name, u8 visibility) {
    FeSymbol* sym = mars_alloc(sizeof(FeSymbol));
    sym->name = name;
    sym->visibility = visibility;

    da_append(&mod->symtab, sym);
    return sym;
}

// use this instead of fe_new_symbol
FeSymbol* fe_find_or_new_symbol(FeModule* mod, string name, u8 visibility) {
    FeSymbol* sym = fe_find_symbol(mod, name);
    return sym ? sym : fe_new_symbol(mod, name, visibility);
}

FeSymbol* fe_find_symbol(FeModule* mod, string name) {
    for_urange(i, 0, mod->symtab.len) {
        if (string_eq(mod->symtab.at[i]->name, name)) {
            return mod->symtab.at[i];
        }
    }
    return NULL;
}

FeBasicBlock* fe_new_basic_block(FeFunction* fn, string name) {
    FeBasicBlock* bb = mars_alloc(sizeof(FeBasicBlock));
    if (!bb) CRASH("mars_alloc failed");

    bb->name = name;
    da_init(bb, 4);

    da_append(&fn->blocks, bb);
    return bb;
}

u32 fe_bb_index(FeFunction* fn, FeBasicBlock* bb) {
    for_urange(i, 0, fn->blocks.len) {
        if (fn->blocks.at[i] != bb) continue;

        return i;
    }

    return UINT32_MAX;
}

FeInst* fe_append(FeBasicBlock* bb, FeInst* ir) {
    ir->bb = bb;
    da_append(bb, ir);
    return ir;
}

FeInst* fe_inst(FeFunction* f, u8 type) {
    if (type >= FE_INSTR_COUNT) type = FE_INVALID;
    FeInst* ir = arena_alloc(&f->alloca, air_sizes[type], 8);
    ir->tag = type;
    ir->T = fe_type(f->mod, FE_VOID, 0);
    ir->number = 0;
    return ir;
}

const size_t air_sizes[] = {
    [FE_INVALID]    = 0,
    [FE_ELIMINATED] = 0,

    [FE_ADD] = sizeof(FeBinop),
    [FE_SUB] = sizeof(FeBinop),
    [FE_MUL] = sizeof(FeBinop),
    [FE_DIV] = sizeof(FeBinop),
    
    [FE_AND]   = sizeof(FeBinop),
    [FE_OR]    = sizeof(FeBinop),
    [FE_NOR]   = sizeof(FeBinop),
    [FE_XOR]   = sizeof(FeBinop),
    [FE_SHL]   = sizeof(FeBinop),
    [FE_LSR]   = sizeof(FeBinop),
    [FE_ASR]   = sizeof(FeBinop),

    [FE_STACKOFFSET] = sizeof(FeStackOffset),
    [FE_GETFIELDPTR] = sizeof(FeGetFieldPtr),
    [FE_GETINDEXPTR] = sizeof(FeGetIndexPtr),

    [FE_LOAD]     = sizeof(FeLoad),
    [FE_VOL_LOAD] = sizeof(FeLoad),

    [FE_STORE]     = sizeof(FeStore),
    [FE_VOL_STORE] = sizeof(FeStore),

    [FE_CONST]      = sizeof(FeConst),
    [FE_LOADSYMBOL] = sizeof(FeLoadSymbol),

    [FE_MOV] = sizeof(FeMov),
    [FE_PHI] = sizeof(FePhi),

    [FE_BRANCH] = sizeof(FeBranch),
    [FE_JUMP]   = sizeof(FeJump),

    [FE_PARAMVAL]  = sizeof(FeParamVal),
    [FE_RETURNVAL] = sizeof(FeReturnVal),

    [FE_RETURN] = sizeof(FeReturn),
};

FeInst* fe_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs) {
    FeBinop* ir = (FeBinop*) fe_inst(f, type);
    
    ir->lhs = lhs;
    ir->rhs = rhs;
    return (FeInst*) ir;
}

FeInst* fe_cast(FeFunction* f, FeInst* source, FeType* to) {
    FeCast* ir = (FeCast*) fe_inst(f, FE_CAST);
    ir->source = source;
    ir->to = to;
    return (FeInst*) ir;
}

FeInst* fe_stackoffset(FeFunction* f, FeStackObject* obj) {
    FeStackOffset* ir = (FeStackOffset*) fe_inst(f, FE_STACKOFFSET);

    ir->object = obj;
    return (FeInst*) ir;
}

FeInst* fe_getfieldptr(FeFunction* f, u32 index, FeInst* source) {
    FeGetFieldPtr* ir = (FeGetFieldPtr*) fe_inst(f, FE_GETFIELDPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_getindexptr(FeFunction* f, FeInst* index, FeInst* source) {
    FeGetIndexPtr* ir = (FeGetIndexPtr*) fe_inst(f, FE_GETINDEXPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_load(FeFunction* f, FeInst* location, bool is_vol) {
    FeLoad* ir = (FeLoad*) fe_inst(f, FE_LOAD);

    if (is_vol) ir->base.tag = FE_VOL_LOAD;
    ir->location = location;
    return (FeInst*) ir;
}

FeInst* fe_store(FeFunction* f, FeInst* location, FeInst* value, bool is_vol) {
    FeStore* ir = (FeStore*) fe_inst(f, FE_STORE);
    
    if (is_vol) ir->base.tag = FE_VOL_STORE;
    ir->location = location;
    ir->value = value;
    return (FeInst*) ir;
}

FeInst* fe_const(FeFunction* f) {
    FeConst* ir = (FeConst*) fe_inst(f, FE_CONST);
    return (FeInst*) ir;
}

FeInst* fe_loadsymbol(FeFunction* f, FeSymbol* symbol) {
    FeLoadSymbol* ir = (FeLoadSymbol*) fe_inst(f, FE_LOADSYMBOL);
    ir->sym = symbol;
    return (FeInst*) ir;
}

FeInst* fe_mov(FeFunction* f, FeInst* source) {
    FeMov* ir = (FeMov*) fe_inst(f, FE_MOV);
    ir->source = source;
    return (FeInst*) ir;
}

// use in the format (f, source_count, source_1, source_BB_1, source_2, source_BB_2, ...)
FeInst* fe_phi(FeFunction* f, u32 count, ...) {
    FePhi* ir = (FePhi*) fe_inst(f, FE_PHI);
    ir->len = count;

    ir->sources    = mars_alloc(sizeof(*ir->sources) * count);
    ir->source_BBs = mars_alloc(sizeof(*ir->source_BBs) * count);

    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        ir->sources[i]    = va_arg(args, FeInst*);
        ir->source_BBs[i] = va_arg(args, FeBasicBlock*);
    }
    va_end(args);

    return (FeInst*) ir;
}

void fe_add_phi_source(FePhi* phi, FeInst* source, FeBasicBlock* source_block) {
    // wrote this and then remembered mars_realloc exists. too late :3
    FeInst** new_sources    = mars_alloc(sizeof(*phi->sources) * (phi->len + 1));
    FeBasicBlock** new_source_BBs = mars_alloc(sizeof(*phi->source_BBs) * (phi->len + 1));

    if (!new_sources || !new_source_BBs) {
        CRASH("mars_alloc returned null");
    }

    memcpy(new_sources, phi->sources, sizeof(*phi->sources) * phi->len);
    memcpy(new_source_BBs, phi->source_BBs, sizeof(*phi->source_BBs) * phi->len);

    new_sources[phi->len]    = source;
    new_source_BBs[phi->len] = source_block;

    mars_free(phi->sources);
    mars_free(phi->source_BBs);

    phi->sources = new_sources;
    phi->source_BBs = new_source_BBs;
    phi->len++;
}

FeInst* fe_jump(FeFunction* f, FeBasicBlock* dest) {
    FeJump* ir = (FeJump*) fe_inst(f, FE_JUMP);
    ir->dest = dest;
    return (FeInst*) ir;
}

FeInst* fe_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false) {
    FeBranch* ir = (FeBranch*) fe_inst(f, FE_BRANCH);
    ir->cond = cond;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (FeInst*) ir;
}

FeInst* fe_paramval(FeFunction* f, u32 param) {
    FeParamVal* ir = (FeParamVal*) fe_inst(f, FE_PARAMVAL);
    ir->param_idx = param;
    return (FeInst*) ir;
}

FeInst* fe_returnval(FeFunction* f, u32 param, FeInst* source) {
    FeReturnVal* ir = (FeReturnVal*) fe_inst(f, FE_RETURNVAL);
    ir->return_idx = param;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_return(FeFunction* f) {
    return fe_inst(f, FE_RETURN);
}

void fe_move_inst(FeBasicBlock* bb, u64 to, u64 from) {
    if (to == from) return;

    FeInst* from_elem = bb->at[from];
    if (to < from) {
        memmove(&bb->at[to+1], &bb->at[to], (from - to) * sizeof(FeInst*));
    } else {
        memmove(&bb->at[to], &bb->at[to+1], (to - from) * sizeof(FeInst*));
    }
    bb->at[to] = from_elem;
}