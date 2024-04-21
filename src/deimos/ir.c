#include "ir.h"

IR_Module* ir_new_module(string name) {
    IR_Module* mod = malloc(sizeof(IR_Module));

    mod->functions = NULL;
    mod->globals   = NULL;
    mod->functions_len = 0;
    mod->globals_len   = 0;

    mod->name = name;

    da_init(&mod->symtab, 4);
    return mod;
}

// if (sym == NULL), create new symbol with no name
IR_Function* ir_new_function(IR_Module* mod, IR_Symbol* sym, bool global) {
    IR_Function* fn = malloc(sizeof(IR_Function));

    fn->sym = sym ? sym : ir_new_symbol(mod, NULL_STR, global, true, fn);
    fn->alloca = arena_make(IR_FN_ALLOCA_BLOCK_SIZE);
    da_init(&fn->blocks, 1);
    fn->entry_idx = 0;
    fn->exit_idx = 0;

    mod->functions = realloc(mod->functions, mod->functions_len+1);
    mod->functions[mod->functions_len++] = fn;
    return fn;
}

// takes multiple type*
void ir_set_func_params(IR_Function* f, u16 count, ...) {
    f->params_len = count;

    if (f->params) free(f->params);

    f->params = malloc(sizeof(IR_FuncItem) * count);

    if (!f->params) CRASH("malloc failed");

    va_list args;
    va_start(args, count);
    FOR_RANGE(i, 0, count) {
        IR_FuncItem* item = malloc(sizeof(IR_FuncItem));
        if (!item) CRASH("item malloc failed");
        item->T = va_arg(args, type*);
        f->params[i] = item;
    }
    va_end(args);
}

// takes multiple type*
void ir_set_func_returns(IR_Function* f, u16 count, ...) {
    f->returns_len = count;

    f->returns = malloc(sizeof(*f->returns) * count);

    va_list args;
    va_start(args, count);
    FOR_RANGE(i, 0, count) {
        IR_FuncItem* item = malloc(sizeof(IR_FuncItem));
        item->T = va_arg(args, type*);
        f->returns[i] = item;
    }
    va_end(args);
}

// if (sym == NULL), create new symbol with default name
IR_Global* ir_new_global(IR_Module* mod, IR_Symbol* sym, bool global, bool read_only) {
    IR_Global* gl = malloc(sizeof(IR_Global));

    gl->sym = sym ? sym : ir_new_symbol(mod, strprintf("symbol%zu", sym), global, false, gl);
    gl->read_only = read_only;
    gl->data = NULL;
    gl->data_len = 0;

    mod->globals = realloc(mod->globals, mod->globals_len+1);
    mod->globals[mod->globals_len++] = gl;
    return gl;
}

void ir_set_global_data(IR_Global* global, u8* data, u32 data_len, bool zeroed) {
    global->is_symbol_ref = false;
    global->data = data;
    global->data_len = data_len;
    global->zeroed = zeroed;
}

void ir_set_global_symref(IR_Global* global, IR_Symbol* symref) {
    global->is_symbol_ref = true;
    global->symref = symref;
}

// WARNING: does NOT check if a symbol already exists
// in most cases, use ir_find_or_create_symbol
IR_Symbol* ir_new_symbol(IR_Module* mod, string name, u8 tag, bool function, void* ref) {
    IR_Symbol* sym = malloc(sizeof(IR_Symbol));
    sym->name = name;
    sym->ref = ref;
    sym->is_function = function;
    sym->tag = tag;

    da_append(&mod->symtab, sym);
    return sym;
}

// use this instead of ir_new_symbol
IR_Symbol* ir_find_or_new_symbol(IR_Module* mod, string name, u8 tag, bool function, void* ref) {
    IR_Symbol* sym = ir_find_symbol(mod, name);
    return sym ? sym : ir_new_symbol(mod, name, tag, function, ref);
}

IR_Symbol* ir_find_symbol(IR_Module* mod, string name) {
    FOR_URANGE(i, 0, mod->symtab.len) {
        if (string_eq(mod->symtab.at[i]->name, name)) {
            return mod->symtab.at[i];
        }
    }
    return NULL;
}

IR_BasicBlock* ir_new_basic_block(IR_Function* fn, string name) {
    IR_BasicBlock* bb = malloc(sizeof(IR_BasicBlock));
    if (!bb) CRASH("malloc failed");

    bb->name = name;
    da_init(bb, 1);

    da_append(&fn->blocks, bb);
    return bb;
}

u32 ir_bb_index(IR_Function* fn, IR_BasicBlock* bb) {
    FOR_URANGE(i, 0, fn->blocks.len) {
        if (fn->blocks.at[i] != bb) continue;

        return i;
    }

    return UINT32_MAX;
}

IR* ir_add(IR_BasicBlock* bb, IR* ir) {
    da_append(bb, ir);
    return ir;
}

IR* ir_make(IR_Function* f, u8 type) {
    if (type > IR_INSTR_COUNT) type = IR_INVALID;
    IR* ir = arena_alloc(&f->alloca, ir_sizes[type], 8);
    ir->tag = type;
    ir->T = NULL;
    ir->number = 0;
    return ir;
}

const size_t ir_sizes[] = {
    [IR_INVALID]    = 0,
    [IR_ELIMINATED] = 0,

    [IR_ADD] = sizeof(IR_BinOp),
    [IR_SUB] = sizeof(IR_BinOp),
    [IR_MUL] = sizeof(IR_BinOp),
    [IR_DIV] = sizeof(IR_BinOp),
    
    [IR_AND]   = sizeof(IR_BinOp),
    [IR_OR]    = sizeof(IR_BinOp),
    [IR_NOR]   = sizeof(IR_BinOp),
    [IR_XOR]   = sizeof(IR_BinOp),
    [IR_SHL]   = sizeof(IR_BinOp),
    [IR_LSR]   = sizeof(IR_BinOp),
    [IR_ASR]   = sizeof(IR_BinOp),
    [IR_TRUNC] = sizeof(IR_BinOp),
    [IR_SEXT]  = sizeof(IR_BinOp),
    [IR_ZEXT]  = sizeof(IR_BinOp),

    [IR_STACKALLOC] = sizeof(IR_StackAlloc),

    [IR_LOAD]     = sizeof(IR_Load),
    [IR_VOL_LOAD] = sizeof(IR_Load),

    [IR_STORE]     = sizeof(IR_Store),
    [IR_VOL_STORE] = sizeof(IR_Store),

    [IR_CONST]      = sizeof(IR_Const),
    [IR_LOADSYMBOL] = sizeof(IR_LoadSymbol),

    [IR_MOV] = sizeof(IR_Mov),
    [IR_PHI] = sizeof(IR_Phi),

    [IR_BRANCH] = sizeof(IR_Branch),
    [IR_JUMP]   = sizeof(IR_Jump),

    [IR_PARAMVAL]  = sizeof(IR_ParamVal),
    [IR_RETURNVAL] = sizeof(IR_ReturnVal),

    [IR_RETURN] = sizeof(IR_Return),
};

IR* ir_make_binop(IR_Function* f, u8 type, IR* lhs, IR* rhs) {
    IR_BinOp* ir = (IR_BinOp*) ir_make(f, type);
    
    ir->lhs = lhs;
    ir->rhs = rhs;
    return (IR*) ir;
}

IR* ir_make_cast(IR_Function* f, IR* source, type* to) {
    IR_Cast* ir = (IR_Cast*) ir_make(f, IR_CAST);
    ir->source = source;
    ir->to = to;
    return (IR*) ir;
}

IR* ir_make_stackalloc(IR_Function* f, u32 size, u32 align, type* T) {
    IR_StackAlloc* ir = (IR_StackAlloc*) ir_make(f, IR_STACKALLOC);
    
    ir->size = size;
    ir->align = align;
    ir->T = T;
    return (IR*) ir;
}

IR* ir_make_getfieldptr(IR_Function* f, u16 index, IR* source) {
    IR_GetFieldPtr* ir = (IR_GetFieldPtr*) ir_make(f, IR_GETFIELDPTR);
    ir->index = index;
    ir->source = source;
    return (IR*) ir;
}

IR* ir_make_load(IR_Function* f, IR* location, type* T, bool is_vol) {
    IR_Load* ir = (IR_Load*) ir_make(f, IR_LOAD);

    if (is_vol) ir->base.tag = IR_VOL_LOAD;
    ir->location = location;
    ir->T = T;
    return (IR*) ir;
}

IR* ir_make_store(IR_Function* f, IR* location, IR* value, type* T, bool is_vol) {
    IR_Store* ir = (IR_Store*) ir_make(f, IR_STORE);
    
    if (is_vol) ir->base.tag = IR_VOL_STORE;
    ir->location = location;
    ir->value = value;
    ir->T = T;
    return (IR*) ir;
}

IR* ir_make_const(IR_Function* f) {
    IR_Const* ir = (IR_Const*) ir_make(f, IR_CONST);
    return (IR*) ir;
}

IR* ir_make_loadsymbol(IR_Function* f, IR_Symbol* symbol) {
    IR_LoadSymbol* ir = (IR_LoadSymbol*) ir_make(f, IR_LOADSYMBOL);
    ir->sym = symbol;
    return (IR*) ir;
}

IR* ir_make_mov(IR_Function* f, IR* source) {
    IR_Mov* ir = (IR_Mov*) ir_make(f, IR_MOV);
    ir->source = source;
    return (IR*) ir;
}

// use in the format (f, source_count, source_1, source_BB_1, source_2, source_BB_2, ...)
IR* ir_make_phi(IR_Function* f, u16 count, ...) {
    IR_Phi* ir = (IR_Phi*) ir_make(f, IR_PHI);
    ir->len = count;

    ir->sources    = malloc(sizeof(*ir->sources) * count);
    ir->source_BBs = malloc(sizeof(*ir->source_BBs) * count);

    va_list args;
    va_start(args, count);
    FOR_RANGE(i, 0, count) {
        ir->sources[i]    = va_arg(args, IR*);
        ir->source_BBs[i] = va_arg(args, IR_BasicBlock*);
    }
    va_end(args);

    return (IR*) ir;
}

void ir_add_phi_source(IR_Phi* phi, IR* source, IR_BasicBlock* source_block) {
    // wrote this and then remembered realloc exists. too late :3
    IR** new_sources    = malloc(sizeof(*phi->sources) * (phi->len + 1));
    IR_BasicBlock** new_source_BBs = malloc(sizeof(*phi->source_BBs) * (phi->len + 1));

    if (!new_sources || !new_source_BBs) {
        CRASH("malloc returned null");
    }

    memcpy(new_sources, phi->sources, sizeof(*phi->sources) * phi->len);
    memcpy(new_source_BBs, phi->source_BBs, sizeof(*phi->source_BBs) * phi->len);

    new_sources[phi->len]    = source;
    new_source_BBs[phi->len] = source_block;

    free(phi->sources);
    free(phi->source_BBs);

    phi->sources = new_sources;
    phi->source_BBs = new_source_BBs;
    phi->len++;
}

IR* ir_make_jump(IR_Function* f, IR_BasicBlock* dest) {
    IR_Jump* ir = (IR_Jump*) ir_make(f, IR_JUMP);
    ir->dest = dest;
    return (IR*) ir;
}

IR* ir_make_branch(IR_Function* f, u8 cond, IR* lhs, IR* rhs, IR_BasicBlock* if_true, IR_BasicBlock* if_false) {
    IR_Branch* ir = (IR_Branch*) ir_make(f, IR_BRANCH);
    ir->cond = cond;
    ir->lhs = lhs;
    ir->rhs = rhs;
    ir->if_true  = if_true;
    ir->if_false = if_false;
    return (IR*) ir;
}

IR* ir_make_paramval(IR_Function* f, u16 param) {
    IR_ParamVal* ir = (IR_ParamVal*) ir_make(f, IR_PARAMVAL);
    ir->param_idx = param;
    return (IR*) ir;
}

IR* ir_make_returnval(IR_Function* f, u16 param, IR* source) {
    IR_ReturnVal* ir = (IR_ReturnVal*) ir_make(f, IR_RETURNVAL);
    ir->return_idx = param;
    ir->source = source;
    return (IR*) ir;
}

IR* ir_make_return(IR_Function* f) {
    return ir_make(f, IR_RETURN);
}


forceinline bool is_type_integer(type* t) {
    switch (t->tag) {
    case TYPE_UNTYPED_INT:
    case TYPE_I8:
    case TYPE_I16:
    case TYPE_I32:
    case TYPE_I64:
    case TYPE_U8:
    case TYPE_U16:
    case TYPE_U32:
    case TYPE_U64:
        return true;
    default:
        return false;
    }
}

forceinline bool is_type_float(type* t) {
    switch (t->tag) {
    case TYPE_F16:
    case TYPE_F32:
    case TYPE_F64:
        return true;
    default:
        return false;
    }
}

forceinline bool is_type_sinteger(type* t) {
    switch (t->tag) {
    case TYPE_UNTYPED_INT:
    case TYPE_I8:
    case TYPE_I16:
    case TYPE_I32:
    case TYPE_I64:
        return true;
    default:
        return false;
    }
}

forceinline bool is_type_uinteger(type* t) {
    switch (t->tag) {
    case TYPE_U8:
    case TYPE_U16:
    case TYPE_U32:
    case TYPE_U64:
        return true;
    default:
        return false;
    }
}

void ir_type_resolve_all(IR_Module* mod, IR_Function* f, bool assume_correct) {
    if (!assume_correct) {
        FOR_URANGE(bb, 0, f->blocks.len) {
            FOR_URANGE(i, 0, f->blocks.at[bb]->len) {
                f->blocks.at[bb]->at[i]->T = NULL;
            }
        }
    }


    FOR_URANGE(bb, 0, f->blocks.len) {
        FOR_URANGE(i, 0, f->blocks.at[bb]->len) {
            ir_type_resolve(mod, f, f->blocks.at[bb]->at[i], true);
        }
    }

}

void ir_type_resolve(IR_Module* mod, IR_Function* f, IR* ir, bool assume_correct) {
    if (ir->T != NULL && assume_correct) return; // if it already has a type, assume that type is correct

    switch (ir->tag) {
    case IR_INVALID:
    case IR_ELIMINATED:
        ir->T = make_type(TYPE_NONE);
        break;
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_AND:
    case IR_OR:
    case IR_NOR:
    case IR_XOR:
        IR_BinOp* binop = (IR_BinOp*) ir;
        ir_type_resolve(mod, f, binop->lhs, true);
        ir_type_resolve(mod, f, binop->rhs, true);
        if (binop->lhs->T != binop->rhs->T) {
            CRASH("type mismatch");
        }
        if (!is_type_integer(binop->lhs->T)) {
            CRASH("type is not integer (TODO work w/ floats)");
        }
        ir->T = binop->lhs->T;
        break;
    case IR_PARAMVAL:
        IR_ParamVal* paramval = (IR_ParamVal*) ir;
        ir->T = f->params[paramval->param_idx]->T;
        break;
    case IR_RETURNVAL:
        IR_ReturnVal* returnval = (IR_ReturnVal*) ir;
        ir->T = f->returns[returnval->return_idx]->T;
        break;
    case IR_RETURN:
        ir->T = make_type(TYPE_NONE);
        break;
    default:
        CRASH("unhandled ir type");
    }

}

u32 ir_renumber(IR_Function* f) {
    u32 count = 0;
    FOR_URANGE(i, 0, f->blocks.len) {
        FOR_URANGE(j, 0, f->blocks.at[i]->len) {
            if (f->blocks.at[i]->at[j]->tag == IR_ELIMINATED) continue;
            f->blocks.at[i]->at[j]->number = ++count;
        }
    }
    return count;
}

void ir_print_function(IR_Function* f) {
    ir_renumber(f);
    
    printf("fn \""str_fmt"\" ", str_arg(f->sym->name));

    printf("(");
    FOR_URANGE(i, 0, f->params_len) {
        string typestr = type_to_string(f->params[i]->T);
        printf(str_fmt, str_arg(typestr));

        if (i + 1 != f->params_len) {
            printf(", ");
        }
    }

    printf(") -> (");
    FOR_URANGE(i, 0, f->returns_len) {
        string typestr = type_to_string(f->returns[i]->T);
        printf(str_fmt, str_arg(typestr));
        
        if (i + 1 != f->returns_len) {
            printf(", ");
        }
    }

    printf(") {\n");
    FOR_URANGE(i, 0, f->blocks.len) {
        printf("    ");
        if (f->entry_idx == i) printf("entry ");
        if (f->entry_idx == i) printf("exit ");
        ir_print_bb(f->blocks.at[i]);
    }
    printf("}\n");
}

void ir_print_bb(IR_BasicBlock* bb) {
    printf(str_fmt":\n", str_arg(bb->name));

    FOR_URANGE(i, 0, bb->len) {
        printf("        ");
        ir_print_ir(bb->at[i]);
        printf("\n");
    }
}

void ir_print_ir(IR* ir) {
    
    if (!ir) {
        printf("[null]");
        return;
    }

    printf("#%zu "str_fmt"\t = ", ir->number, str_arg(type_to_string(ir->T)));
    switch (ir->tag) {
    case IR_INVALID: 
        printf("invalid"); 
        break;

    case IR_ELIMINATED: 
        return;

    case IR_ADD: char* binopstr = "add"; goto print_binop;
    case IR_SUB: binopstr = "sub"; goto print_binop;
    case IR_MUL: binopstr = "mul"; goto print_binop;
    case IR_DIV: binopstr = "div"; goto print_binop;
    case IR_AND: binopstr = "and"; goto print_binop;
    case IR_OR:  binopstr = "or"; goto print_binop;
    case IR_NOR: binopstr = "nor"; goto print_binop;
    case IR_XOR: binopstr = "xor"; goto print_binop;
    case IR_SHL: binopstr = "shl"; goto print_binop;
    case IR_LSR: binopstr = "lsr"; goto print_binop;
    case IR_ASR: binopstr = "asr"; goto print_binop;
    case IR_TRUNC: binopstr = "trunc"; goto print_binop;
    case IR_SEXT: binopstr = "sext"; goto print_binop;
    case IR_ZEXT: binopstr = "zext"; goto print_binop;
        print_binop:
        IR_BinOp* binop = (IR_BinOp*) ir;
        printf("%s #%zu, #%zu", binopstr, binop->lhs->number, binop->rhs->number);
        break;
    
    case IR_PARAMVAL:
        IR_ParamVal* paramval = (IR_ParamVal*) ir;
        printf("paramval <%zu>", paramval->param_idx);
        break;

    case IR_RETURNVAL:
        IR_ReturnVal* returnval = (IR_ReturnVal*) ir;
        printf("returnval <%zu> #%zu", returnval->return_idx, returnval->source->number);
        break;

    case IR_RETURN:
        printf("return");
        break;

    default:
        printf("unimplemented %zu", (size_t)ir->tag);
        break;
    }
}

void ir_print_module(IR_Module* mod) {
    printf("module \""str_fmt"\"\n", str_arg(mod->name));
    printf("-> %zu functions:\n", mod->functions_len);
}

static char* write_str(char* buf, char* src) {
    memcpy(buf, src, strlen(src));
    return buf + strlen(src);
}

static char* type2str_internal(type* t, char* buf) {
    if (t == NULL) {
        buf = write_str(buf, "[null]"); 
        return buf;
    }

    switch (t->tag) {
    case TYPE_NONE: return buf;
    case TYPE_BOOL: buf = write_str(buf, "bool"); return buf;
    case TYPE_U8:   buf = write_str(buf, "u8");  return buf;
    case TYPE_U16:  buf = write_str(buf, "u16"); return buf;
    case TYPE_U32:  buf = write_str(buf, "u32"); return buf;
    case TYPE_U64:  buf = write_str(buf, "u64"); return buf;
    case TYPE_I8:   buf = write_str(buf, "i8");  return buf;
    case TYPE_I16:  buf = write_str(buf, "i16"); return buf;
    case TYPE_I32:  buf = write_str(buf, "i32"); return buf;
    case TYPE_I64:  buf = write_str(buf, "i64"); return buf;
    case TYPE_F16:  buf = write_str(buf, "f16"); return buf;
    case TYPE_F32:  buf = write_str(buf, "f32"); return buf;
    case TYPE_F64:  buf = write_str(buf, "f64"); return buf;  
    case TYPE_POINTER:  buf = write_str(buf, "ptr"); return buf;  
    default: buf = write_str(buf, "unimpl"); return buf;
    }
}

string type_to_string(type* t) {
    // get a backing buffer
    char buf[500];
    memset(buf, 0, sizeof(buf));
    type2str_internal(t, buf);
    // allocate new buffer with concrete backing
    return string_clone(str(buf));
}