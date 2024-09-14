#include "iron/iron.h"

#include "iron/passes/passes.h"

// if (sym == NULL), create new symbol with no name
FeFunction* fe_new_function(FeModule* mod, FeSymbol* sym) {
    FeFunction* fn = mars_alloc(sizeof(FeFunction));

    fn->sym = sym ? sym : fe_new_symbol(mod, NULL_STR, FE_BIND_EXPORT);
    if (!sym) fn->sym->name = strprintf("symbol_%016llx", fn->sym);
    fn->sym->is_function = true;
    fn->sym->function = fn;

    fn->alloca = arena_make(FE_FN_ALLOCA_BLOCK_SIZE);
    da_init(&fn->blocks, 1);
    da_init(&fn->stack, 1);
    fn->params.at = NULL;
    fn->returns.at = NULL;
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

static char* cstrprintf(char* fmt, ...) {

    va_list varargs;
    va_start(varargs, fmt);

    static char buf[1000];
    memset(buf, 0, sizeof(buf));

    vsprintf(buf, fmt, varargs);
    char* out = malloc(strlen(buf));
    strcpy(out, buf);
    return out;
}

void fe_init_func_params(FeFunction* f, u16 count) {
    f->params.at = mars_alloc(count * sizeof(f->params.at[0]));
    f->params.cap = count;
    f->params.len = 0;
}
void fe_init_func_returns(FeFunction* f, u16 count) {
    f->returns.at = mars_alloc(count * sizeof(f->returns.at[0]));
    f->returns.cap = count;
    f->returns.len = 0;
}
FeFunctionItem* fe_add_func_param(FeFunction* f, FeType* t) {
    if (f->params.at == NULL) {
        fe_init_func_params(f, 4);
    }
    if (f->params.len == f->params.cap) {
        f->params.at = mars_realloc(f->params.at, f->params.cap * 2);
        f->params.cap *= 2;
    }
    FeFunctionItem* p = mars_alloc(sizeof(*p));
    p->type = t;
    f->params.at[f->params.len++] = p;
    return p;
}
FeFunctionItem* fe_add_func_return(FeFunction* f, FeType* t) {
    if (f->returns.at == NULL) {
        fe_init_func_params(f, 4);
    }
    if (f->returns.len == f->returns.cap) {
        f->returns.at = mars_realloc(f->returns.at, f->returns.cap * 2);
        f->returns.cap *= 2;
    }
    FeFunctionItem* r = mars_alloc(sizeof(*r));
    r->type = t;
    f->returns.at[f->returns.len++] = r;
    return r;
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
FeSymbol* fe_new_symbol(FeModule* mod, string name, u8 binding) {
    FeSymbol* sym = mars_alloc(sizeof(FeSymbol));
    sym->name = name;
    sym->binding = binding;

    da_append(&mod->symtab, sym);
    return sym;
}

FeSymbol* fe_find_or_new_symbol(FeModule* mod, string name, u8 binding) {
    FeSymbol* sym = fe_find_symbol(mod, name);
    return sym ? sym : fe_new_symbol(mod, name, binding);
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

    bb->name = name;
    bb->function = fn;
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

FeInst* fe_insert_inst(FeBasicBlock* bb, FeInst* inst, i64 index) {
    inst->bb = bb;
    if (bb->at[index]->kind == FE_INST_ELIMINATED) {
        bb->at[index] = inst;
    } else {
        da_insert_at(bb, inst, index);
    }
    return inst;
}

i64 fe_index_of_inst(FeBasicBlock* bb, FeInst* inst) {
    for_range(i, 0, bb->len) {
        if (bb->at[i] == inst) {
            return i;
        }
    }
    return -1;
}

// inserts inst before ref
FeInst* fe_insert_inst_before(FeBasicBlock* bb, FeInst* inst, FeInst* ref) {
    return fe_insert_inst(bb, inst, fe_index_of_inst(bb, ref));
}

// inserts inst after ref
FeInst* fe_insert_inst_after(FeBasicBlock* bb, FeInst* inst, FeInst* ref) {
    return fe_insert_inst(bb, inst, fe_index_of_inst(bb, ref) + 1);
}

FeInst* fe_inst(FeFunction* f, u8 type) {
    if (type >= _FE_INST_MAX) type = FE_INST_INVALID;
    FeInst* ir = arena_alloc(&f->alloca, fe_inst_sizes[type], 8);
    ir->kind = type;
    ir->type = fe_type(f->mod, FE_TYPE_VOID);
    ir->number = 0;
    return ir;
}

const size_t fe_inst_sizes[] = {
    [FE_INST_INVALID]    = sizeof(FeInst),
    [FE_INST_ELIMINATED] = sizeof(FeInst),

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

    [FE_INST_NOT]     = sizeof(FeInstUnop),
    [FE_INST_NEG]     = sizeof(FeInstUnop),
    [FE_INST_CAST]    = sizeof(FeInstUnop),
    [FE_INST_BITCAST] = sizeof(FeInstUnop),

    [FE_INST_STACKADDR] = sizeof(FeInstStackAddr),
    [FE_INST_FIELDPTR]  = sizeof(FeInstFieldPtr),
    [FE_INST_INDEXPTR]  = sizeof(FeInstIndexPtr),

    [FE_INST_LOAD]        = sizeof(FeInstLoad),
    [FE_INST_VOL_LOAD]    = sizeof(FeInstLoad),
    [FE_INST_STACK_LOAD] = sizeof(FeInstStackLoad),

    [FE_INST_STORE]       = sizeof(FeInstStore),
    [FE_INST_VOL_STORE]   = sizeof(FeInstStore),
    [FE_INST_STACK_STORE] = sizeof(FeInstStackStore),

    [FE_INST_CONST]  = sizeof(FeInstConst),
    [FE_INST_LOAD_SYMBOL] = sizeof(FeLoadSymbol),

    [FE_INST_MOV] = sizeof(FeInstMov),
    [FE_INST_PHI] = sizeof(FeInstPhi),

    [FE_INST_BRANCH] = sizeof(FeInstBranch),
    [FE_INST_JUMP]   = sizeof(FeInstJump),

    [FE_INST_PARAMVAL]  = sizeof(FeInstParamVal),
    [FE_INST_RETURNVAL] = sizeof(FeInstReturnval),

    [FE_INST_RETURN] = sizeof(FeInstReturn),
};

#define FE_FATAL(m, msg) fe_push_message(m, (FeMessage){ \
    .function_of_origin = __func__,\
    .message = (msg),\
    .severity = FE_MSG_SEVERITY_FATAL, \
})

FeInst* fe_inst_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs) {
    FeInstBinop* ir = (FeInstBinop*) fe_inst(f, type);

    if (lhs->type == rhs->type) {
        ir->base.type = lhs->type;
    } else {
        FE_FATAL(f->mod, "lhs type != rhs type");
    }

    switch (type) {
    case FE_INST_ADD:
    case FE_INST_UMUL:
    case FE_INST_IMUL:
        if (lhs->kind == FE_INST_CONST && rhs->kind != FE_INST_CONST) {
            ir->lhs = rhs;
            ir->rhs = lhs;
            break;
        }
    default:
        ir->lhs = lhs;
        ir->rhs = rhs;
        break;
    }

    return (FeInst*) ir;
}

FeInst* fe_inst_unop(FeFunction* f, u8 type, FeInst* source) {
    FeInstUnop* ir = (FeInstUnop*) fe_inst(f, type);
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_inst_stackaddr(FeFunction* f, FeStackObject* obj) {
    FeInstStackAddr* ir = (FeInstStackAddr*) fe_inst(f, FE_INST_STACKADDR);

    ir->object = obj;
    return (FeInst*) ir;
}

FeInst* fe_inst_getfieldptr(FeFunction* f, u32 index, FeInst* source) {
    FeInstFieldPtr* ir = (FeInstFieldPtr*) fe_inst(f, FE_INST_FIELDPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_inst_getindexptr(FeFunction* f, FeInst* index, FeInst* source) {
    FeInstIndexPtr* ir = (FeInstIndexPtr*) fe_inst(f, FE_INST_INDEXPTR);
    ir->index = index;
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_inst_load(FeFunction* f, FeInst* ptr, FeType* as, bool is_vol) {
    FeInstLoad* ir = (FeInstLoad*) fe_inst(f, FE_INST_LOAD);
    if (!fe_type_is_scalar(as)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    if (is_vol) ir->base.kind = FE_INST_VOL_LOAD;
    ir->location = ptr;
    ir->base.type = as;
    return (FeInst*) ir;
}

FeInst* fe_inst_store(FeFunction* f, FeInst* ptr, FeInst* value, bool is_vol) {
    FeInstStore* ir = (FeInstStore*) fe_inst(f, FE_INST_STORE);
    if (!fe_type_is_scalar(value->type)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    if (is_vol) ir->base.kind = FE_INST_VOL_STORE;
    ir->location = ptr;
    ir->value = value;
    return (FeInst*) ir;
}

FeInst* fe_inst_stack_load(FeFunction* f, FeStackObject* location) {
    FeInstStackLoad* ir = (FeInstStackLoad*) fe_inst(f, FE_INST_STACK_LOAD);
    if (!fe_type_is_scalar(location->t)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    ir->base.type = location->t;
    ir->location = location;
    return (FeInst*) ir;
}

FeInst* fe_inst_stack_store(FeFunction* f, FeStackObject* location, FeInst* value) {
    FeInstStackStore* ir = (FeInstStackStore*) fe_inst(f, FE_INST_STACK_STORE);
    if (!fe_type_is_scalar(location->t)) {
        FE_FATAL(f->mod, "cannot load/store non-scalar type");
    }
    if (location->t != value->type) {
        FE_FATAL(f->mod, "value type != location type");
    }
    ir->location = location;
    ir->value = value;
    return (FeInst*) ir;
}

FeInst* fe_inst_const(FeFunction* f, FeType* type) {
    FeInstConst* ir = (FeInstConst*) fe_inst(f, FE_INST_CONST);
    ir->base.type = type;
    return (FeInst*) ir;
}

FeInst* fe_inst_load_symbol(FeFunction* f, FeType* type, FeSymbol* symbol) {
    FeLoadSymbol* ir = (FeLoadSymbol*) fe_inst(f, FE_INST_LOAD_SYMBOL);
    ir->sym = symbol;
    if (!fe_type_is_scalar(type)) {
        FE_FATAL(f->mod, "cannot load symbol into non-scalar type");
    }
    return (FeInst*) ir;
}

FeInst* fe_inst_mov(FeFunction* f, FeInst* source) {
    FeInstMov* ir = (FeInstMov*) fe_inst(f, FE_INST_MOV);
    ir->source = source;
    ir->base.type = source->type;
    return (FeInst*) ir;
}

FeInst* fe_inst_phi(FeFunction* f, u32 count, FeType* type) {
    FeInstPhi* ir = (FeInstPhi*) fe_inst(f, FE_INST_PHI);
    ir->base.type = type;
    ir->len = 0;
    ir->cap = count;
    ir->source_BBs = mars_alloc(sizeof(*ir->source_BBs)*count);
    ir->sources = mars_alloc(sizeof(*ir->sources)*count);
    return (FeInst*) ir;
}

void fe_add_phi_source(FeInstPhi* phi, FeInst* source, FeBasicBlock* source_block) {
    TODO("finish this");
}

FeInst* fe_inst_jump(FeFunction* f, FeBasicBlock* dest) {
    FeInstJump* ir = (FeInstJump*) fe_inst(f, FE_INST_JUMP);
    ir->dest = dest;
    return (FeInst*) ir;
}

FeInst* fe_inst_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false) {
    FeInstBranch* ir = (FeInstBranch*) fe_inst(f, FE_INST_BRANCH);
    ir->cond = cond;
    if (cond == 0 || cond > FE_COND_NE) {
        FE_FATAL(f->mod, cstrprintf("invalid condition %d", cond));
    }
    if (lhs->type != rhs->type) {
        FE_FATAL(f->mod, cstrprintf("lhs type != rhs type", cond));
    }
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (FeInst*) ir;
}

FeInst* fe_inst_paramval(FeFunction* f, u32 param) {
    FeInstParamVal* ir = (FeInstParamVal*) fe_inst(f, FE_INST_PARAMVAL);
    ir->param_idx = param;
    if (param >= f->params.len) {
        FE_FATAL(f->mod, cstrprintf("paramval index %d is out of range [0, %d)", param, f->params.len));
        // CRASH("paramval index %d is out of range", param);
    }
    ir->base.type = f->params.at[param]->type;
    return (FeInst*) ir;
}

FeInst* fe_inst_returnval(FeFunction* f, u32 ret, FeInst* source) {
    FeInstReturnval* ir = (FeInstReturnval*) fe_inst(f, FE_INST_RETURNVAL);
    ir->return_idx = ret;
    if (ret >= f->returns.len) {
        FE_FATAL(f->mod, cstrprintf("returnval index %d is out of range [0, %d)", ret, f->returns.len));
    }
    ir->source = source;
    return (FeInst*) ir;
}

FeInst* fe_inst_return(FeFunction* f) {
    return fe_inst(f, FE_INST_RETURN);
}

void fe_move(FeBasicBlock* bb, u64 to, u64 from) {
    if (to == from) return;

    FeInst* from_elem = bb->at[from];
    if (to < from) {
        memmove(&bb->at[to+1], &bb->at[to], (from - to) * sizeof(FeInst*));
    } else {
        memmove(&bb->at[to], &bb->at[to+1], (to - from) * sizeof(FeInst*));
    }
    bb->at[to] = from_elem;
}

// rewrite all uses of `from` to be uses of `to`

static u64 set_usage(FeBasicBlock* bb, FeInst* source, u64 start_index, FeInst* dest) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->kind == FE_INST_ELIMINATED) continue;
        // FIXME: kayla you're gonna be SO fucking mad at me for this
        // searching the struct for a pointer :sobbing:
        FeInst** ir = (FeInst**)bb->at[i];
        for (u64 j = sizeof(FeInst)/sizeof(FeInst*); j <= fe_inst_sizes[bb->at[i]->kind]/sizeof(FeInst*); j++) {
            if (ir[j] == source) {
                ir[j] = dest;
                return i;
            }
        }
    }
    return UINT64_MAX;
}

void fe_rewrite_uses(FeFunction* f, FeInst* source, FeInst* dest) {
    for_urange(i, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[i];
        u64 next_usage = set_usage(bb, source, 0, dest);
        while (next_usage != UINT64_MAX) {
            next_usage = set_usage(bb, source, next_usage+1, dest);
        }
    }
}

static u64 get_usage(FeBasicBlock* bb, FeInst* source, u64 start_index) {
    for (u64 i = start_index; i < bb->len; i++) {
        if (bb->at[i]->kind == FE_INST_ELIMINATED) continue;
        FeInst** ir = (FeInst**)bb->at[i];
        for (u64 j = sizeof(FeInst)/sizeof(FeInst*); j <= fe_inst_sizes[bb->at[i]->kind]/sizeof(FeInst*); j++) {
            if (ir[j] == source) return i;
        }
    }
    return UINT64_MAX;
}

void fe_add_uses_to_worklist(FeFunction* f, FeInst* source, da(FeInstPTR)* worklist) {
    for_urange(i, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[i];
        u64 next_usage = get_usage(bb, source, 0);
        while (next_usage != UINT64_MAX) {
            da_append(worklist, bb->at[next_usage]);
            next_usage = get_usage(bb, source, next_usage+1);
        }
    }
}