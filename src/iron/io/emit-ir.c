#include "iron/iron.h"
#include "common/strbuilder.h"

static bool fancy = false;

static char* simpletype2cstr(FeType* t) {
    switch (t->kind){
    case FE_TYPE_VOID: return "void";
    case FE_TYPE_BOOL: return "bool";
    case FE_TYPE_I8:   return "i8";
    case FE_TYPE_I16:  return "i16";
    case FE_TYPE_I32:  return "i32";
    case FE_TYPE_I64:  return "i64";
    case FE_TYPE_F16:  return "f16";
    case FE_TYPE_F32:  return "f32";
    case FE_TYPE_F64:  return "f64";
    case FE_TYPE_PTR:  return "ptr";
    }
    return NULL;
}

static u64 stack_object_index(FeFunction* f, FeStackObject* obj) {
    for_urange(i, 0, f->stack.len) {
        FeStackObject* so = f->stack.at[i];
        if (so == obj) return i;
    }
    return UINT64_MAX;
}

static void emit_sym(StringBuilder* sb, FeSymbol* sym) {
    sb_append_c(sb, "(sym \"");
    sb_append(sb, sym->name);
    sb_append_c(sb, "\" ");
    switch (sym->binding) {
    case FE_BIND_LOCAL: sb_append_c(sb, "local"); break;
    case FE_BIND_IMPORT: sb_append_c(sb, "import"); break;
    case FE_BIND_EXPORT: sb_append_c(sb, "export"); break;
    case FE_BIND_EXPORT_WEAK: sb_append_c(sb, "export-weak"); break;
    default: CRASH("unknown symbol binding"); break;
    }
    sb_append_c(sb, ")");
}

static u64 symbol_index(FeModule* m, FeSymbol* sym) {
    for_urange(i, 0, m->symtab.len) {
        if (m->symtab.at[i] == sym) return i;
    }
    return UINT64_MAX;
}

static void emit_type(StringBuilder* sb, FeType* t) {
    if (_FE_TYPE_SIMPLE_BEGIN < t->kind && t->kind < _FE_TYPE_SIMPLE_END) {
        sb_append_c(sb, simpletype2cstr(t));
    } else if (t->kind == FE_TYPE_RECORD) {
        sb_append_c(sb, "(rec ");
        for_range(i, 0, t->aggregate.len) {
            emit_type(sb, t->aggregate.fields[i]);
            if (i != t->aggregate.len - 1) sb_append_c(sb, " ");
        }
        sb_append_c(sb, ")");
    } else if (t->kind == FE_TYPE_ARRAY) {
        sb_append_c(sb, "(arr ");
        sb_printf(sb, "%llx ", t->array.len);
        emit_type(sb, t->array.sub);
        sb_append_c(sb, ")");
    } else {
        CRASH("invalid type");
    }
}

static void emit_inst(StringBuilder* sb, FeInst* inst) {

    static const char* opnames[] = {
        [FE_INST_ADD]   = "add ",
        [FE_INST_SUB]   = "sub ",
        [FE_INST_IMUL]  = "imul ",
        [FE_INST_UMUL]  = "umul ",
        [FE_INST_IDIV]  = "idiv ",
        [FE_INST_UDIV]  = "udiv ",
        [FE_INST_AND]   = "and ",
        [FE_INST_OR]    = "or ",
        [FE_INST_XOR]   = "xor ",
        [FE_INST_SHL]   = "shl ",
        [FE_INST_LSR]   = "lsr ",
        [FE_INST_ASR]   = "asr ",
        [FE_INST_NEG]   = "neg ",
        [FE_INST_NOT]   = "not ",
    };

    sb_append_c(sb, "\n      (");
    switch (inst->kind) {
    case FE_INST_PARAMVAL:
        FeInstParamVal* paramval = (FeInstParamVal*) inst;
        sb_printf(sb, "paramval %u", paramval->param_idx);
        break;
    case FE_INST_RETURNVAL:
        FeInstReturnval* returnval = (FeInstReturnval*) inst;
        sb_printf(sb, "returnval %u %u", returnval->return_idx, returnval->source->number);
        break;
    case FE_INST_RETURN:
        sb_append_c(sb, "return");
        break;
    case FE_INST_ADD:
    case FE_INST_SUB:
    case FE_INST_IMUL:
    case FE_INST_UMUL:
    case FE_INST_IDIV:
    case FE_INST_UDIV:
    case FE_INST_AND:
    case FE_INST_OR:
    case FE_INST_XOR:
    case FE_INST_SHL:
    case FE_INST_LSR:
    case FE_INST_ASR:
        FeInstBinop* binop = (FeInstBinop*) inst;
        sb_append_c(sb, opnames[inst->kind]);
        emit_type(sb, inst->type);
        sb_printf(sb, " %u %u", binop->lhs->number, binop->rhs->number);
        break;
    default:
        break;
    }
    sb_append_c(sb, ")");
}

static void emit_function(StringBuilder* sb, FeFunction* f) {

    {
        int counter = 0;
        foreach(FeBasicBlock* bb, f->blocks) {
            foreach(FeInst* inst, *bb) {
                inst->number = counter++;
            }
        }
    }

    sb_append_c(sb, "(fun ");
    sb_printf(sb, "%llu ", symbol_index(f->mod, f->sym));

    sb_append_c(sb, "(");
    for_range(i, 0, f->params_len) {
        if (f->params[i]->by_value) TODO("");
        FeType* t = f->params[i]->type;
        emit_type(sb, t);
        if (i != f->params_len - 1) sb_append_c(sb, " ");
    }
    sb_append_c(sb, ") (");
    for_range(i, 0, f->returns_len) {
        if (f->returns[i]->by_value) TODO("");
        FeType* t = f->returns[i]->type;
        emit_type(sb, t);
        if (i != f->returns_len - 1) sb_append_c(sb, " ");
    }
    sb_append_c(sb, ") \n");

    for_range(i, 0, f->stack.len) {
        sb_append_c(sb, "(stk ");
        FeType* t = f->stack.at[i]->t;
        emit_type(sb, t);
        sb_append_c(sb, ") ");
    }    

    foreach(FeBasicBlock* bb, f->blocks) {
        sb_append_c(sb, "    (blk ");
        sb_append(sb, bb->name);

        foreach(FeInst* inst, *bb) {
            emit_inst(sb, inst);
        }

        sb_append_c(sb, ")");
    }

    sb_append_c(sb, ")");
}

// `fancy_whitespace` enables line breaks and indentation.
string fe_emit_ir(FeModule* m, bool fancy_whitespace) {
    fancy = fancy_whitespace;
    StringBuilder sb = {0};
    sb_init(&sb);

    sb_append_c(&sb, "(mod \"");
    sb_append(&sb, m->name);
    sb_append_c(&sb, "\"");
    foreach(FeSymbol* sym, m->symtab) {
        sb_append_c(&sb, "\n  ");
        emit_sym(&sb, sym);
    }
    for_range(i, 0, m->functions_len) {
        sb_append_c(&sb, "\n  ");
        emit_function(&sb, m->functions[i]);

    }

    sb_append_c(&sb, ")\n");

    string out = string_alloc(sb_len(&sb));
    sb_write(&sb, out.raw);
    sb_destroy(&sb);
    return out;
}