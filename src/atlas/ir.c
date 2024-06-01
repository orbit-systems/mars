#include "atlas.h"

AIR_Module* air_new_module(AtlasModule* am) {
    AIR_Module* mod = mars_alloc(sizeof(*mod));

    am->ir_module = mod;

    da_init(&mod->symtab, 16);

    air_typegraph_init(am);

    return mod;
}

// if (sym == NULL), create new symbol with no name
AIR_Function* air_new_function(AIR_Module* mod, AIR_Symbol* sym, u8 visibility) {
    AIR_Function* fn = mars_alloc(sizeof(AIR_Function));

    fn->sym = sym ? sym : air_new_symbol(mod, NULL_STR, visibility, true, fn);
    fn->alloca = arena_make(AIR_FN_ALLOCA_BLOCK_SIZE);
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

AIR_StackObject* air_new_stackobject(AIR_Function* f, AIR_Type* t) {
    AIR_StackObject* obj = arena_alloc(&f->alloca, sizeof(*obj), alignof(*obj));
    obj->t = t;
    da_append(&f->stack, obj);
    return obj;
}

// takes multiple AIR_Type*
void air_set_func_params(AIR_Function* f, u16 count, ...) {
    f->params_len = count;

    if (f->params) mars_free(f->params);

    f->params = mars_alloc(sizeof(*f->params) * count);
    if (!f->params) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        AIR_FuncItem* item = mars_alloc(sizeof(AIR_FuncItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->T = va_arg(args, AIR_Type*);
            if (item->T == NULL) {
                no_set = true;
            }
        }
        
        f->params[i] = item;
    }
    va_end(args);
}

// takes multiple entity*
void air_set_func_returns(AIR_Function* f, u16 count, ...) {
    f->returns_len = count;

    if (f->returns) mars_free(f->returns);

    f->returns = mars_alloc(sizeof(*f->returns) * count);
    if (!f->params) CRASH("mars_alloc failed");

    bool no_set = false;
    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        AIR_FuncItem* item = mars_alloc(sizeof(AIR_FuncItem));
        if (!item) CRASH("item mars_alloc failed");
        
        if (!no_set) {
            item->T = va_arg(args, AIR_Type*);
            if (item->T == NULL) {
                no_set = true;
            }
        }
        
        f->returns[i] = item;
    }
    va_end(args);
}

// if (sym == NULL), create new symbol with default name
AIR_Global* air_new_global(AIR_Module* mod, AIR_Symbol* sym, bool global, bool read_only) {
    AIR_Global* gl = mars_alloc(sizeof(AIR_Global));

    gl->sym = sym ? sym : air_new_symbol(mod, strprintf("symbol%zu", sym), global, false, gl);
    gl->read_only = read_only;
    gl->data = NULL;
    gl->data_len = 0;

    mod->globals = mars_realloc(mod->globals, sizeof(*mod->globals) * (mod->globals_len+1));
    mod->globals[mod->globals_len++] = gl;
    return gl;
}

void air_set_global_data(AIR_Global* global, u8* data, u32 data_len, bool zeroed) {
    global->is_symbol_ref = false;
    global->data = data;
    global->data_len = data_len;
    global->zeroed = zeroed;
}

void air_set_global_symref(AIR_Global* global, AIR_Symbol* symref) {
    global->is_symbol_ref = true;
    global->symref = symref;
}

// WARNING: does NOT check if a symbol already exists
// in most cases, use air_find_or_create_symbol
AIR_Symbol* air_new_symbol(AIR_Module* mod, string name, u8 visibility, bool function, void* ref) {
    AIR_Symbol* sym = mars_alloc(sizeof(AIR_Symbol));
    sym->name = name;
    sym->ref = ref;
    sym->is_function = function;
    sym->visibility = visibility;

    da_append(&mod->symtab, sym);
    return sym;
}

// use this instead of air_new_symbol
AIR_Symbol* air_find_or_new_symbol(AIR_Module* mod, string name, u8 visibility, bool function, void* ref) {
    AIR_Symbol* sym = air_find_symbol(mod, name);
    return sym ? sym : air_new_symbol(mod, name, visibility, function, ref);
}

AIR_Symbol* air_find_symbol(AIR_Module* mod, string name) {
    for_urange(i, 0, mod->symtab.len) {
        if (string_eq(mod->symtab.at[i]->name, name)) {
            return mod->symtab.at[i];
        }
    }
    return NULL;
}

AIR_BasicBlock* air_new_basic_block(AIR_Function* fn, string name) {
    AIR_BasicBlock* bb = mars_alloc(sizeof(AIR_BasicBlock));
    if (!bb) CRASH("mars_alloc failed");

    bb->name = name;
    da_init(bb, 4);

    da_append(&fn->blocks, bb);
    return bb;
}

u32 air_bb_index(AIR_Function* fn, AIR_BasicBlock* bb) {
    for_urange(i, 0, fn->blocks.len) {
        if (fn->blocks.at[i] != bb) continue;

        return i;
    }

    return UINT32_MAX;
}

AIR* air_add(AIR_BasicBlock* bb, AIR* ir) {
    ir->bb = bb;
    // da_append(bb, ir);
    if ((bb)->len == (bb)->cap) {
        (bb)->cap *= 2;
        printf("--wuh-- %d %d\n", bb->len, bb->cap);
        (bb)->at = realloc((bb)->at, sizeof((bb)->at[0]) * (bb)->cap);
        if ((bb)->at == NULL) {
            printf("(%s:%d) da_append realloc failed for capacity %zu", (__FILE__), (__LINE__), (bb)->cap);
            exit(1);
        }
        printf("--wuhaa-- %d %d\n", bb->len, bb->cap);
    }
    (bb)->at[(bb)->len++] = (ir);
    return ir;
}

AIR* air_make(AIR_Function* f, u8 type) {
    if (type > AIR_INSTR_COUNT) type = AIR_INVALID;
    AIR* ir = arena_alloc(&f->alloca, air_sizes[type], 8);
    ir->tag = type;
    ir->T = air_new_type(f->mod->am, AIR_VOID, 0);
    ir->number = 0;
    return ir;
}

const size_t air_sizes[] = {
    [AIR_INVALID]    = 0,
    [AIR_ELIMINATED] = 0,

    [AIR_ADD] = sizeof(AIR_BinOp),
    [AIR_SUB] = sizeof(AIR_BinOp),
    [AIR_MUL] = sizeof(AIR_BinOp),
    [AIR_DIV] = sizeof(AIR_BinOp),
    
    [AIR_AND]   = sizeof(AIR_BinOp),
    [AIR_OR]    = sizeof(AIR_BinOp),
    [AIR_NOR]   = sizeof(AIR_BinOp),
    [AIR_XOR]   = sizeof(AIR_BinOp),
    [AIR_SHL]   = sizeof(AIR_BinOp),
    [AIR_LSR]   = sizeof(AIR_BinOp),
    [AIR_ASR]   = sizeof(AIR_BinOp),

    [AIR_STACKOFFSET] = sizeof(AIR_StackOffset),
    [AIR_GETFIELDPTR] = sizeof(AIR_GetFieldPtr),
    [AIR_GETINDEXPTR] = sizeof(AIR_GetIndexPtr),

    [AIR_LOAD]     = sizeof(AIR_Load),
    [AIR_VOL_LOAD] = sizeof(AIR_Load),

    [AIR_STORE]     = sizeof(AIR_Store),
    [AIR_VOL_STORE] = sizeof(AIR_Store),

    [AIR_CONST]      = sizeof(AIR_Const),
    [AIR_LOADSYMBOL] = sizeof(AIR_LoadSymbol),

    [AIR_MOV] = sizeof(AIR_Mov),
    [AIR_PHI] = sizeof(AIR_Phi),

    [AIR_BRANCH] = sizeof(AIR_Branch),
    [AIR_JUMP]   = sizeof(AIR_Jump),

    [AIR_PARAMVAL]  = sizeof(AIR_ParamVal),
    [AIR_RETURNVAL] = sizeof(AIR_ReturnVal),

    [AIR_RETURN] = sizeof(AIR_Return),
};

AIR* air_make_binop(AIR_Function* f, u8 type, AIR* lhs, AIR* rhs) {
    AIR_BinOp* ir = (AIR_BinOp*) air_make(f, type);
    
    ir->lhs = lhs;
    ir->rhs = rhs;
    return (AIR*) ir;
}

AIR* air_make_cast(AIR_Function* f, AIR* source, AIR_Type* to) {
    AIR_Cast* ir = (AIR_Cast*) air_make(f, AIR_CAST);
    ir->source = source;
    ir->to = to;
    return (AIR*) ir;
}

AIR* air_make_stackoffset(AIR_Function* f, AIR_StackObject* obj) {
    AIR_StackOffset* ir = (AIR_StackOffset*) air_make(f, AIR_STACKOFFSET);

    ir->object = obj;
    return (AIR*) ir;
}

AIR* air_make_getfieldptr(AIR_Function* f, u32 index, AIR* source) {
    AIR_GetFieldPtr* ir = (AIR_GetFieldPtr*) air_make(f, AIR_GETFIELDPTR);
    ir->index = index;
    ir->source = source;
    return (AIR*) ir;
}

AIR* air_make_getindexptr(AIR_Function* f, AIR* index, AIR* source) {
    AIR_GetIndexPtr* ir = (AIR_GetIndexPtr*) air_make(f, AIR_GETINDEXPTR);
    ir->index = index;
    ir->source = source;
    return (AIR*) ir;
}

AIR* air_make_load(AIR_Function* f, AIR* location, bool is_vol) {
    AIR_Load* ir = (AIR_Load*) air_make(f, AIR_LOAD);

    if (is_vol) ir->base.tag = AIR_VOL_LOAD;
    ir->location = location;
    return (AIR*) ir;
}

AIR* air_make_store(AIR_Function* f, AIR* location, AIR* value, bool is_vol) {
    AIR_Store* ir = (AIR_Store*) air_make(f, AIR_STORE);
    
    if (is_vol) ir->base.tag = AIR_VOL_STORE;
    ir->location = location;
    ir->value = value;
    return (AIR*) ir;
}

AIR* air_make_const(AIR_Function* f) {
    AIR_Const* ir = (AIR_Const*) air_make(f, AIR_CONST);
    return (AIR*) ir;
}

AIR* air_make_loadsymbol(AIR_Function* f, AIR_Symbol* symbol) {
    AIR_LoadSymbol* ir = (AIR_LoadSymbol*) air_make(f, AIR_LOADSYMBOL);
    ir->sym = symbol;
    return (AIR*) ir;
}

AIR* air_make_mov(AIR_Function* f, AIR* source) {
    AIR_Mov* ir = (AIR_Mov*) air_make(f, AIR_MOV);
    ir->source = source;
    return (AIR*) ir;
}

// use in the format (f, source_count, source_1, source_BB_1, source_2, source_BB_2, ...)
AIR* air_make_phi(AIR_Function* f, u32 count, ...) {
    AIR_Phi* ir = (AIR_Phi*) air_make(f, AIR_PHI);
    ir->len = count;

    ir->sources    = mars_alloc(sizeof(*ir->sources) * count);
    ir->source_BBs = mars_alloc(sizeof(*ir->source_BBs) * count);

    va_list args;
    va_start(args, count);
    for_range(i, 0, count) {
        ir->sources[i]    = va_arg(args, AIR*);
        ir->source_BBs[i] = va_arg(args, AIR_BasicBlock*);
    }
    va_end(args);

    return (AIR*) ir;
}

void air_add_phi_source(AIR_Phi* phi, AIR* source, AIR_BasicBlock* source_block) {
    // wrote this and then remembered mars_realloc exists. too late :3
    AIR** new_sources    = mars_alloc(sizeof(*phi->sources) * (phi->len + 1));
    AIR_BasicBlock** new_source_BBs = mars_alloc(sizeof(*phi->source_BBs) * (phi->len + 1));

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

AIR* air_make_jump(AIR_Function* f, AIR_BasicBlock* dest) {
    AIR_Jump* ir = (AIR_Jump*) air_make(f, AIR_JUMP);
    ir->dest = dest;
    return (AIR*) ir;
}

AIR* air_make_branch(AIR_Function* f, u8 cond, AIR* lhs, AIR* rhs, AIR_BasicBlock* if_true, AIR_BasicBlock* if_false) {
    AIR_Branch* ir = (AIR_Branch*) air_make(f, AIR_BRANCH);
    ir->cond = cond;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (AIR*) ir;
}

AIR* air_make_paramval(AIR_Function* f, u32 param) {
    AIR_ParamVal* ir = (AIR_ParamVal*) air_make(f, AIR_PARAMVAL);
    ir->param_idx = param;
    return (AIR*) ir;
}

AIR* air_make_returnval(AIR_Function* f, u32 param, AIR* source) {
    AIR_ReturnVal* ir = (AIR_ReturnVal*) air_make(f, AIR_RETURNVAL);
    ir->return_idx = param;
    ir->source = source;
    return (AIR*) ir;
}

AIR* air_make_return(AIR_Function* f) {
    return air_make(f, AIR_RETURN);
}

void air_move_element(AIR_BasicBlock* bb, u64 to, u64 from) {
    if (to == from) return;

    AIR* from_elem = bb->at[from];
    if (to < from) {
        memmove(&bb->at[to+1], &bb->at[to], (from - to) * sizeof(AIR*));
    } else {
        memmove(&bb->at[to], &bb->at[to+1], (to - from) * sizeof(AIR*));
    }
    bb->at[to] = from_elem;
}


u32 air_renumber(AIR_Function* f) {
    u32 count = 0;
    for_urange(i, 0, f->blocks.len) {
        for_urange(j, 0, f->blocks.at[i]->len) {
            f->blocks.at[i]->at[j]->number = ++count;
        }
    }
    return count;
}

void air_print_function(AIR_Function* f) {
    air_renumber(f);
    
    printf("fn \""str_fmt"\" ", str_arg(f->sym->name));

    printf("(");
    for_urange(i, 0, f->params_len) {
        // string typestr = type_to_string(f->params[i]->e->entity_type);
        // printf(str_fmt, str_arg(typestr));

        if (i + 1 != f->params_len) {
            printf(", ");
        }
    }

    printf(") -> (");
    for_urange(i, 0, f->returns_len) {
        // string typestr = type_to_string(f->returns[i]->e->entity_type);
        // printf(str_fmt, str_arg(typestr));
        
        if (i + 1 != f->returns_len) {
            printf(", ");
        }
    }

    printf(") {\n");
    for_urange(i, 0, f->blocks.len) {
        printf("    ");
        if (f->entry_idx == i) printf("entry ");
        air_print_bb(f->blocks.at[i]);
    }
    printf("}\n");
}

void air_print_bb(AIR_BasicBlock* bb) {
    printf(str_fmt":\n", str_arg(bb->name));

    for_urange(i, 0, bb->len) {
        printf("        ");
        air_print_ir(bb->at[i]);
        printf("\n");
    }
}

void air_print_ir(AIR* ir) {
    
    if (!ir) {
        printf("[null]");
        return;
    }

    char* binopstr; //hacky but w/e

    string typestr = constr("TODO TYPE2STR");//type_to_string(ir->T);
    printf("#%-3zu %-4.*s = ", ir->number, str_arg(typestr));
    switch (ir->tag) {
    case AIR_INVALID: 
        printf("invalid!");
        break;

    case AIR_ELIMINATED:
        printf("---");
        return;

    if(0){case AIR_ADD:   binopstr = "add";} //common path fallthroughs
    if(0){case AIR_SUB:   binopstr = "sub";}
    if(0){case AIR_MUL:   binopstr = "mul";}
    if(0){case AIR_DIV:   binopstr = "div";}
    if(0){case AIR_AND:   binopstr = "and";}
    if(0){case AIR_OR:    binopstr = "or";} 
    if(0){case AIR_NOR:   binopstr = "nor";}
    if(0){case AIR_XOR:   binopstr = "xor";}
    if(0){case AIR_SHL:   binopstr = "shl";}
    if(0){case AIR_LSR:   binopstr = "lsr";}
    if(0){case AIR_ASR:   binopstr = "asr";}
        AIR_BinOp* binop = (AIR_BinOp*) ir;
        printf("%s #%zu, #%zu", binopstr, binop->lhs->number, binop->rhs->number);
        break;
    
    case AIR_PARAMVAL:
        AIR_ParamVal* paramval = (AIR_ParamVal*) ir;
        printf("paramval <%zu>", paramval->param_idx);
        break;

    case AIR_RETURNVAL:
        AIR_ReturnVal* returnval = (AIR_ReturnVal*) ir;
        printf("returnval <%zu> #%zu", returnval->return_idx, returnval->source->number);
        break;

    case AIR_RETURN:
        printf("return");
        break;

    case AIR_STACKOFFSET:
        AIR_StackOffset* stackoffset = (AIR_StackOffset*) ir;
        // string typestr = type_to_string(stackalloc->alloctype);
        // CRASH("");
        string typestr = str("TODO");
        printf("stackoffset <"str_fmt">", str_arg(typestr));
        break;

    case AIR_STORE:
        AIR_Store* store = (AIR_Store*) ir;
        printf("store #%zu, #%zu", store->location->number, store->value->number);
        break;
    
    case AIR_LOAD:
        AIR_Load* load = (AIR_Load*) ir;
        printf("load #%zu", load->location->number);
        break;

    case AIR_MOV:
        AIR_Mov* mov = (AIR_Mov*) ir;
        printf("mov #%zu", mov->source->number);
        break;

    case AIR_CONST:
        AIR_Const* con = (AIR_Const*) ir;

        // string typestr_ = type_to_string(con->base.T);
        // printf("const <"str_fmt", ", str_arg(typestr_));

        switch (con->base.T->kind) {
        case AIR_I8:  printf("%lld", (i64)con->i8);  break;
        case AIR_I16: printf("%lld", (i64)con->i16); break;
        case AIR_I32: printf("%lld", (i64)con->i32); break;
        case AIR_I64: printf("%lld", (i64)con->i64); break;
        case AIR_U8:  printf("%llu", (u64)con->u8);  break;
        case AIR_U16: printf("%llu", (u64)con->u16); break;
        case AIR_U32: printf("%llu", (u64)con->u32); break;
        case AIR_U64: printf("%llu", (u64)con->u64); break;
        case AIR_F16: printf("%f",   (f32)con->f16); break;
        case AIR_F32: printf("%f",   con->f32); break;
        case AIR_F64: printf("%lf",  con->f64); break;
        default:
            printf("???");
            break;
        }
        printf(">");
        break;

    default:
        printf("unimplemented %zu", (size_t)ir->tag);
        break;
    }
}

void air_print_module(AIR_Module* mod) {
    for_urange(i, 0, mod->functions_len) {
        air_print_function(mod->functions[i]);
    }
}

static char* write_str(char* buf, char* src) {
    memcpy(buf, src, strlen(src));
    return buf + strlen(src);
}