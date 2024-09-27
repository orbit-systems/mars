#include "iron/iron.h"
#include "common/strbuilder.h"

#define COLORFUL

#ifdef COLORFUL
    #define STYLE_Reset  "\x1b[0m"
    #define STYLE_Bold   "\x1b[1m"
    #define STYLE_Dim    "\x1b[2m"
    #define STYLE_Italic "\x1b[3m"

    #define STYLE_FG_Black    "\x1b[30m"
    #define STYLE_FG_Red      "\x1b[31m"
    #define STYLE_FG_Green    "\x1b[32m"
    #define STYLE_FG_Yellow   "\x1b[33m"
    #define STYLE_FG_Blue     "\x1b[34m"
    #define STYLE_FG_Magenta  "\x1b[35m"
    #define STYLE_FG_Cyan     "\x1b[36m"
    #define STYLE_FG_White    "\x1b[37m"
    #define STYLE_FG_Default  "\x1b[39m"

    #define STYLE_BG_Black    "\x1b[40m"
    #define STYLE_BG_Red      "\x1b[41m"
    #define STYLE_BG_Green    "\x1b[42m"
    #define STYLE_BG_Yellow   "\x1b[43m"
    #define STYLE_BG_Blue     "\x1b[44m"
    #define STYLE_BG_Magenta  "\x1b[45m"
    #define STYLE_BG_Cyan     "\x1b[46m"
    #define STYLE_BG_White    "\x1b[47m"
    #define STYLE_BG_Default  "\x1b[49m"
#else
    #define STYLE_Reset  ""
    #define STYLE_Bold   ""
    #define STYLE_Dim    ""
    #define STYLE_Italic ""

    #define STYLE_FG_Black    ""
    #define STYLE_FG_Red      ""
    #define STYLE_FG_Green    ""
    #define STYLE_FG_Yellow   ""
    #define STYLE_FG_Blue     ""
    #define STYLE_FG_Magenta  ""
    #define STYLE_FG_Cyan     ""
    #define STYLE_FG_White    ""
    #define STYLE_FG_Default  ""

    #define STYLE_BG_Black    ""
    #define STYLE_BG_Red      ""
    #define STYLE_BG_Green    ""
    #define STYLE_BG_Yellow   ""
    #define STYLE_BG_Blue     ""
    #define STYLE_BG_Magenta  ""
    #define STYLE_BG_Cyan     ""
    #define STYLE_BG_White    ""
    #define STYLE_BG_Default  ""
#endif

#define COLOR_SYMBOL STYLE_FG_Green
#define COLOR_STACK  STYLE_FG_Cyan
#define COLOR_INST   STYLE_FG_Red
#define COLOR_TYPE   STYLE_FG_Yellow
#define RESET        STYLE_Reset

static char* simpletype2cstr(FeType t) {
    switch (t){
    case FE_TYPE_VOID: return COLOR_TYPE"void"RESET;
    case FE_TYPE_BOOL: return COLOR_TYPE"bool"RESET;
    case FE_TYPE_I8:   return COLOR_TYPE"i8"RESET;
    case FE_TYPE_I16:  return COLOR_TYPE"i16"RESET;
    case FE_TYPE_I32:  return COLOR_TYPE"i32"RESET;
    case FE_TYPE_I64:  return COLOR_TYPE"i64"RESET;
    case FE_TYPE_F16:  return COLOR_TYPE"f16"RESET;
    case FE_TYPE_F32:  return COLOR_TYPE"f32"RESET;
    case FE_TYPE_F64:  return COLOR_TYPE"f64"RESET;
    case FE_TYPE_PTR:  return COLOR_TYPE"ptr"RESET;
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
    sb_append_c(sb, "(sym \'");
    sb_append(sb, sym->name);
    sb_append_c(sb, "\' ");
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

static void emit_data(StringBuilder* sb, FeModule* m, FeData* data) {
    sb_printf(sb, "(dat "COLOR_SYMBOL"%llu "RESET, symbol_index(m, data->sym));
    if (data->kind == FE_DATA_SYMREF) {
        sb_printf(sb, "symref "COLOR_SYMBOL"%llu"RESET, symbol_index(m, data->symref));
    }
    sb_append_c(sb, ")");
}

static void emit_type(StringBuilder* sb, FeModule* m, FeType t) {
    FeComplexType* ct = fe_type_get_structure(m, t);
    if (t < _FE_TYPE_SIMPLE_END) {
        sb_append_c(sb, simpletype2cstr(t));
    // } else if (t->kind == FE_TYPE_RECORD) {
    } else if (ct->kind == FE_TYPE_RECORD) {
        sb_append_c(sb, "(rec ");
        for_range(i, 0, ct->record.len) {
            emit_type(sb, m, ct->record.fields[i]);
            if (i != ct->record.len - 1) sb_append_c(sb, " ");
        }
        sb_append_c(sb, ")");
    } else if (ct->kind == FE_TYPE_ARRAY) {
        sb_append_c(sb, "(arr ");
        sb_printf(sb, "%llx ", ct->array.len);
        emit_type(sb, m, ct->array.sub);
        sb_append_c(sb, ")");
    } else {
        CRASH("invalid type");
    }
}

static void emit_inst(StringBuilder* sb, FeFunction* fn, FeInst* inst) {

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

        [FE_INST_ULT]   = "ult ",
        [FE_INST_UGT]   = "ugt ",
        [FE_INST_ULE]   = "ule ",
        [FE_INST_UGE]   = "uge ",
        [FE_INST_ILT]   = "ilt ",
        [FE_INST_IGT]   = "igt ",
        [FE_INST_ILE]   = "ile ",
        [FE_INST_IGE]   = "ige ",
        [FE_INST_EQ]    = "eq ",
        [FE_INST_NE]    = "ne ",
    };

    // sb_append_c(sb, "\n      (");
    sb_printf(sb, "\n"COLOR_INST"     % 9llu: "RESET"(", inst->number);
    switch (inst->kind) {
    case FE_INST_PARAMVAL:
        FeInstParamVal* paramval = (FeInstParamVal*) inst;
        sb_printf(sb, "paramval %u", paramval->param_idx);
        break;
    case FE_INST_RETURNVAL:
        FeInstReturnval* returnval = (FeInstReturnval*) inst;
        sb_printf(sb, "returnval %u"COLOR_INST" %u"RESET, returnval->return_idx, returnval->source->number);
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
        emit_type(sb, fn->mod, inst->type);
        sb_printf(sb, COLOR_INST" %u %u"RESET, binop->lhs->number, binop->rhs->number);
        break;
    case FE_INST_ULT:
    case FE_INST_UGT:
    case FE_INST_ULE:
    case FE_INST_UGE:
    case FE_INST_ILT:
    case FE_INST_IGT:
    case FE_INST_ILE:
    case FE_INST_IGE:
    case FE_INST_EQ:
    case FE_INST_NE:
        FeInstBinop* cmp = (FeInstBinop*) inst;
        sb_append_c(sb, opnames[inst->kind]);
        sb_printf(sb, COLOR_INST"%u %u"RESET, cmp->lhs->number, cmp->rhs->number);
        break;
    
    case FE_INST_CONST:
        FeInstConst* const_inst = (FeInstConst*) inst;
        sb_append_c(sb, "const ");
        emit_type(sb, fn->mod, inst->type);
        sb_printf(sb, " %llu", const_inst->i64);
        break;
    case FE_INST_STACK_STORE:
        FeInstStackStore* stack_store = (FeInstStackStore*) inst;
        sb_printf(sb, "stack_store "COLOR_STACK"%u"COLOR_INST" %u"RESET, 
            stack_object_index(fn, stack_store->location),
            stack_store->value->number
        );
        break;
    case FE_INST_STACK_LOAD:
        FeInstStackLoad* stack_load = (FeInstStackLoad*) inst;
        sb_printf(sb, "stack_load "COLOR_STACK"%u"RESET, stack_object_index(fn, stack_load->location));
        break;

    case FE_INST_BRANCH:
        FeInstBranch* branch = (FeInstBranch*) inst;
        sb_printf(sb, "branch "COLOR_INST"%u"RESET" \'"str_fmt"\' \'"str_fmt"\'", 
            branch->cond->number, 
            str_arg(branch->if_true->name),
            str_arg(branch->if_false->name));
        break;
    
    case FE_INST_JUMP:
        FeInstJump* jump = (FeInstJump*) inst;
        sb_printf(sb, "jump \'"str_fmt"\'", str_arg(jump->dest->name));
        break;
    default:
        sb_append_c(sb, "unknown");
        break;
    }
    sb_append_c(sb, ")");
}

static void emit_function(StringBuilder* sb, FeFunction* f) {

    {
        int counter = 0;
        foreach(FeBasicBlock* bb, f->blocks) {
            for_fe_inst(inst, *bb) {
                inst->number = counter++;
            }
        }
    }

    sb_append_c(sb, "(fun ");
    sb_printf(sb, COLOR_SYMBOL"%llu "RESET, symbol_index(f->mod, f->sym));

    sb_append_c(sb, "(");
    for_range(i, 0, f->params.len) {
        if (f->params.at[i]->by_value) TODO("");
        FeType t = f->params.at[i]->type;
        emit_type(sb, f->mod, t);
        if (i != f->params.len - 1) sb_append_c(sb, " ");
    }
    sb_append_c(sb, ") (");
    for_range(i, 0, f->returns.len) {
        if (f->returns.at[i]->by_value) TODO("");
        FeType t = f->returns.at[i]->type;
        emit_type(sb, f->mod, t);
        if (i != f->returns.len - 1) sb_append_c(sb, " ");
    }
    sb_append_c(sb, ") \n");

    for_range(i, 0, f->stack.len) {
        sb_printf(sb, COLOR_STACK"% 6llu: "RESET, i);
        sb_append_c(sb, "(stk ");
        FeType t = f->stack.at[i]->t;
        emit_type(sb, f->mod, t);
        sb_append_c(sb, ") \n");
    }    

    foreach(FeBasicBlock* bb, f->blocks) {
        sb_append_c(sb, "        (blk \'");
        sb_append(sb, bb->name);
        sb_append_c(sb, "\'");
        for_fe_inst(inst, *bb) {
            emit_inst(sb, f, inst);
        }

        sb_append_c(sb, ")\n");
    }

    sb_append_c(sb, ")");
}

// `fancy_whitespace` enables line breaks and indentation.
string fe_emit_ir(FeModule* m) {
    StringBuilder sb = {0};
    sb_init(&sb);

    sb_append_c(&sb, "(mod \'");
    sb_append(&sb, m->name);
    sb_append_c(&sb, "\'");
    foreach(FeSymbol* sym, m->symtab) {
        // sb_append_c(&sb, "\n    ");COLOR_SYMBOL
        sb_printf(&sb, COLOR_SYMBOL"\n% 2llu: "RESET, symbol_index(m, sym));
        emit_sym(&sb, sym);
    }
    for_range(i, 0, m->datas_len) {
        sb_append_c(&sb, "\n    ");
        emit_data(&sb, m, m->datas[i]);
    }
    for_range(i, 0, m->functions_len) {
        sb_append_c(&sb, "\n    ");
        emit_function(&sb, m->functions[i]);
    }

    sb_append_c(&sb, ")\n");

    string out = string_alloc(sb_len(&sb));
    sb_write(&sb, out.raw);
    sb_destroy(&sb);
    return out;
}