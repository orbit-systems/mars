#include "iron/iron.h"

#include "iron/passes/passes.h"

// if (sym == NULL), create new symbol with no name
FeFunction* fe_new_function(FeModule* mod, FeSymbol* sym) {
    FeFunction* fn = mars_alloc(sizeof(FeFunction));

    fn->sym = sym ? sym : fe_new_symbol(mod, NULL_STR, FE_VIS_GLOBAL);
    if (!sym) fn->sym->name = strprintf("symbol_%016llx", fn->sym);
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
            item->type = va_arg(args, FeType*);
            if (item->type == NULL) {
                no_set = true;
            }
        }
        
        f->params[i] = item;
    }
    va_end(args);
}

// takes multiple FeType*
void fe_set_func_returns(FeFunction* f, u16 count, ...) {
    f->returns_len = count;

    if (f->returns) mars_free(f->returns);

    f->returns = mars_alloc(sizeof(*f->returns) * count);
    if (!f->returns) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        FeFunctionItem* item = mars_alloc(sizeof(FeFunctionItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->type = va_arg(args, FeType*);
            if (item->type == NULL) {
                no_set = true;
            }
        }
        
        f->returns[i] = item;
    }
    va_end(args);
}

FeData* fe_new_data(FeModule* mod, FeSymbol* sym, bool read_only) {
    FeData* data = mars_alloc(sizeof(FeData));

    data->sym = sym;
    data->read_only = read_only;

    mod->datas = mars_realloc(mod->datas, sizeof(*mod->datas) * (mod->datas_len+1));
    mod->datas[mod->datas_len++] = data;
    return data;
}

void fe_set_data_bytes(FeData* data, u8* bytes, u32 len, bool zeroed) {
    data->kind = FE_DATA_BYTES;
    data->bytes.data = bytes;
    data->bytes.len = len;
    data->bytes.zeroed = zeroed;
}

void fe_set_data_symref(FeData* data, FeSymbol* symref) {
    data->kind = FE_DATA_SYMREF;
    data->symref = symref;
}

void fe_set_data_numeric(FeData* data, u64 content, u8 kind) {
    if (!(kind > _FE_DATA_NUMERIC_BEGIN && kind < _FE_DATA_NUMERIC_END)) {
        CRASH("data kind %d is not numeric", kind);
    }
    data->kind = kind;
}

// WARNING: does NOT check if a symbol already exists
FeSymbol* fe_new_symbol(FeModule* mod, string name, u8 visibility) {
    FeSymbol* sym = mars_alloc(sizeof(FeSymbol));
    sym->name = name;
    sym->visibility = visibility;

    da_append(&mod->symtab, sym);
    return sym;
}

FeSymbol* fe_find_or_new_symbol(FeModule* mod, string name, u8 visibility) {
    FeSymbol* sym = fe_find_symbol(mod, name);
    return sym ? sym : fe_new_symbol(mod, name, visibility);
}

// returns NULL if the symbol cannot be found
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

FeInst* fe_append(FeBasicBlock* bb, FeInst* inst) {
    inst->bb = bb;
    da_append(bb, inst);
    return inst;
}

FeInst* fe_insert(FeBasicBlock* bb, FeInst* inst, i64 index) {
    inst->bb = bb;
    da_insert_at(bb, inst, index);
    return inst;
}

i64 fe_index_of_inst(FeBasicBlock* bb, FeInst* inst) {
    for_range(i, 0, bb->len) {
        if (bb->at[i] == inst) {
            return i;
        }
    }
}

// inserts inst before ref
FeInst* fe_insert_before(FeBasicBlock* bb, FeInst* inst, FeInst* ref) {
    return fe_insert(bb, inst, fe_index_of_inst(bb, ref));
}

// inserts inst after ref
FeInst* fe_insert_after(FeBasicBlock* bb, FeInst* inst, FeInst* ref) {
    return fe_insert(bb, inst, fe_index_of_inst(bb, ref) + 1);
}

FeInst* fe_inst(FeFunction* f, u8 type) {
    if (type >= _FE_INST_COUNT) type = FE_INST_INVALID;
    FeInst* ir = arena_alloc(&f->alloca, fe_inst_sizes[type], 8);
    ir->kind = type;
    ir->type = fe_type(f->mod, FE_VOID, 0);
    ir->number = 0;
    return ir;
}

const size_t fe_inst_sizes[] = {
    [FE_INST_INVALID]    = 0,
    [FE_INST_ELIMINATED] = 0,

    [FE_INST_ADD]  = sizeof(FeInstBinop),
    [FE_INST_SUB]  = sizeof(FeInstBinop),
    [FE_INST_IMUL] = sizeof(FeInstBinop),
    [FE_INST_UMUL] = sizeof(FeInstBinop),
    [FE_INST_IDIV] = sizeof(FeInstBinop),
    [FE_INST_UDIV] = sizeof(FeInstBinop),
    
    [FE_INST_AND]  = sizeof(FeInstBinop),
    [FE_INST_OR]   = sizeof(FeInstBinop),
    [FE_INST_XOR]  = sizeof(FeInstBinop),
    [FE_INST_SHL]  = sizeof(FeInstBinop),
    [FE_INST_LSR]  = sizeof(FeInstBinop),
    [FE_INST_ASR]  = sizeof(FeInstBinop),

    [FE_INST_NOT]  = sizeof(FeInstUnop),
    [FE_INST_NEG]  = sizeof(FeInstUnop),
    [FE_INST_CAST] = sizeof(FeInstUnop),

    [FE_INST_STACKADDR] = sizeof(FeInstStackAddr),
    [FE_INST_FIELDPTR]  = sizeof(FeInstFieldPtr),
    [FE_INST_INDEXPTR]  = sizeof(FeInstIndexPtr),

    [FE_INST_LOAD]     = sizeof(FeInstLoad),
    [FE_INST_VOL_LOAD] = sizeof(FeInstLoad),

    [FE_INST_STORE]     = sizeof(FeInstStore),
    [FE_INST_VOL_STORE] = sizeof(FeInstStore),

    [FE_INST_CONST]  = sizeof(FeInstLoadConst),
    [FE_INST_LOAD_SYMBOL] = sizeof(FeLoadSymbol),

    [FE_INST_MOV] = sizeof(FeInstMov),
    [FE_INST_PHI] = sizeof(FeInstPhi),

    [FE_INST_BRANCH] = sizeof(FeInstBranch),
    [FE_INST_JUMP]   = sizeof(FeInstJump),

    [FE_INST_PARAMVAL]  = sizeof(FeInstParamVal),
    [FE_INST_RETURNVAL] = sizeof(FeInstReturnVal),

    [FE_INST_RETURN] = sizeof(FeInstReturn),
};

FeInst* fe_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs) {
    FeInstBinop* ir = (FeInstBinop*) fe_inst(f, type);
    
    ir->lhs = lhs;
    ir->rhs = rhs;
    return (FeInst*) ir;
}

FeInst* fe_unop(FeFunction* f, u8 type, FeInst* source) {
    FeInstUnop* ir = (FeInstUnop*) fe_inst(f, type);
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_stackaddr(FeFunction* f, FeStackObject* obj) {
    FeInstStackAddr* ir = (FeInstStackAddr*) fe_inst(f, FE_INST_STACKADDR);

    ir->object = obj;
    return (FeInst*) ir;
}

FeInst* fe_getfieldptr(FeFunction* f, u32 index, FeInst* source) {
    FeInstFieldPtr* ir = (FeInstFieldPtr*) fe_inst(f, FE_INST_FIELDPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_getindexptr(FeFunction* f, FeInst* index, FeInst* source) {
    FeInstIndexPtr* ir = (FeInstIndexPtr*) fe_inst(f, FE_INST_INDEXPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_load(FeFunction* f, FeInst* location, bool is_vol) {
    FeInstLoad* ir = (FeInstLoad*) fe_inst(f, FE_INST_LOAD);

    if (is_vol) ir->base.kind = FE_INST_VOL_LOAD;
    ir->location = location;
    return (FeInst*) ir;
}

FeInst* fe_store(FeFunction* f, FeInst* location, FeInst* value, bool is_vol) {
    FeInstStore* ir = (FeInstStore*) fe_inst(f, FE_INST_STORE);
    
    if (is_vol) ir->base.kind = FE_INST_VOL_STORE;
    ir->location = location;
    ir->value = value;
    return (FeInst*) ir;
}

FeInst* fe_const(FeFunction* f) {
    FeInstLoadConst* ir = (FeInstLoadConst*) fe_inst(f, FE_INST_CONST);
    return (FeInst*) ir;
}

FeInst* fe_load_symbol(FeFunction* f, FeSymbol* symbol) {
    FeLoadSymbol* ir = (FeLoadSymbol*) fe_inst(f, FE_INST_LOAD_SYMBOL);
    ir->sym = symbol;
    return (FeInst*) ir;
}

FeInst* fe_mov(FeFunction* f, FeInst* source) {
    FeInstMov* ir = (FeInstMov*) fe_inst(f, FE_INST_MOV);
    ir->source = source;
    return (FeInst*) ir;
}

// use in the format (f, source_count, source_1, source_BB_1, source_2, source_BB_2, ...)
FeInst* fe_phi(FeFunction* f, u32 count, ...) {
    FeInstPhi* ir = (FeInstPhi*) fe_inst(f, FE_INST_PHI);
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

void fe_add_phi_source(FeInstPhi* phi, FeInst* source, FeBasicBlock* source_block) {
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
    FeInstJump* ir = (FeInstJump*) fe_inst(f, FE_INST_JUMP);
    ir->dest = dest;
    return (FeInst*) ir;
}

FeInst* fe_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false) {
    FeInstBranch* ir = (FeInstBranch*) fe_inst(f, FE_INST_BRANCH);
    ir->cond = cond;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (FeInst*) ir;
}

FeInst* fe_paramval(FeFunction* f, u32 param) {
    FeInstParamVal* ir = (FeInstParamVal*) fe_inst(f, FE_INST_PARAMVAL);
    ir->param_idx = param;
    return (FeInst*) ir;
}

FeInst* fe_returnval(FeFunction* f, u32 param, FeInst* source) {
    FeInstReturnVal* ir = (FeInstReturnVal*) fe_inst(f, FE_INST_RETURNVAL);
    ir->return_idx = param;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_return(FeFunction* f) {
    return fe_inst(f, FE_INST_RETURN);
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