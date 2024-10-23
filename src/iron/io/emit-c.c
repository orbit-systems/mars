#include "iron/iron.h"

#include "common/ptrmap.h"
#include "common/strbuilder.h"

// not extensively tested, probably incorrect
// just a funny little project
// do NOT USE THIS FOR PRODUCTION ANYTHING
// sincerely, sandwichman

static void emit_prelude(FeModule* m, StringBuilder* sb) {
    sb_append_c(sb, "#include <stdint.h>\n"
                    "typedef uint64_t  u64;\n"
                    "typedef uint32_t  u32;\n"
                    "typedef uint16_t  u16;\n"
                    "typedef uint8_t   u8;\n"
                    "typedef int64_t   i64;\n"
                    "typedef int32_t   i32;\n"
                    "typedef int16_t   i16;\n"
                    "typedef int8_t    i8;\n"
                    "typedef double    f64;\n"
                    "typedef float     f32;\n"
                    "typedef _Float16  f16;\n"
                    "typedef uint8_t   bool;\n"
                    "typedef uint8_t*  ptr;\n"
                    "#define false     ((bool)0)\n"
                    "#define true      ((bool)1)\n"
                    "\n");
}

PtrMap sym2ident = {0};

// normalize a symbol name into a valid C identifier
static string normalized_identifier(FeModule* m, void* entity, string name) {
    if (sym2ident.keys == NULL) ptrmap_init(&sym2ident, m->symtab.len * 2);

    {
        string* s = ptrmap_get(&sym2ident, entity);
        if (s != PTRMAP_NOT_FOUND) {
            return *s;
        }
    }

    bool is_normal = true;
    for_range(i, 0, name.len) {
        char c = name.raw[i];
        if ('a' <= c && c <= 'z') continue;
        if ('A' <= c && c <= 'A') continue;
        if ('0' <= c && c <= '9') continue;
        if (c == '_') continue;
        is_normal = false;
        break;
    }

    if (is_normal) {
        string* permanent_string = malloc(sizeof(string));
        *permanent_string = name;
        ptrmap_put(&sym2ident, entity, permanent_string);
        return name;
    }

    string new_name = string_concat(constr("_"), name);

    for_range(i, 0, new_name.len) {
        char c = new_name.raw[i];
        if ('a' <= c && c <= 'z') continue;
        if ('A' <= c && c <= 'A') continue;
        if ('0' <= c && c <= '9') continue;
        if (c == '_') continue;
        new_name.raw[i] = '_'; // set unrecognized chars to _
    }

    return new_name;
}

// takes in i8, emits (u8), etc.
static char* signed_to_unsigned_cast(FeType t) {
    switch (t) {
    case FE_TYPE_I8: return "(u8)";
    case FE_TYPE_I16: return "(u16)";
    case FE_TYPE_I32: return "(u32)";
    case FE_TYPE_I64: return "(u64)";
    default:
        CRASH("");
    }
}

static void emit_type_name(FeType t, StringBuilder* sb) {
    switch (t) {
    case FE_TYPE_VOID: sb_append_c(sb, "void "); break;
    case FE_TYPE_PTR: sb_append_c(sb, "ptr "); break;
    case FE_TYPE_I8: sb_append_c(sb, "i8 "); break;
    case FE_TYPE_I16: sb_append_c(sb, "i16 "); break;
    case FE_TYPE_I32: sb_append_c(sb, "i32 "); break;
    case FE_TYPE_I64: sb_append_c(sb, "i64 "); break;
    case FE_TYPE_F16: sb_append_c(sb, "f16 "); break;
    case FE_TYPE_F32: sb_append_c(sb, "f32 "); break;
    case FE_TYPE_F64: sb_append_c(sb, "f64 "); break;
    case FE_TYPE_BOOL: sb_append_c(sb, "bool "); break;
    default: sb_printf(sb, "_type_%p ", t); break;
    }
}

static void emit_type_ptr(FeType t, StringBuilder* sb) {

    switch (t) {
    case FE_TYPE_PTR: sb_append_c(sb, "ptr* "); break;
    case FE_TYPE_I8: sb_append_c(sb, "i8* "); break;
    case FE_TYPE_I16: sb_append_c(sb, "i16* "); break;
    case FE_TYPE_I32: sb_append_c(sb, "i32* "); break;
    case FE_TYPE_I64: sb_append_c(sb, "i64* "); break;
    case FE_TYPE_F16: sb_append_c(sb, "f16* "); break;
    case FE_TYPE_F32: sb_append_c(sb, "f32* "); break;
    case FE_TYPE_F64: sb_append_c(sb, "f64* "); break;
    case FE_TYPE_BOOL: sb_append_c(sb, "bool* "); break;
    default: sb_printf(sb, "_type_%p* ", t); break;
    }
}

/*

if it returns a single value, we can actually return it

    func a_function (i64) -> (i64) {...}

becomes

    i64 a_function (i64 _param_0) {...}


if it returns multiple values, return the first value, but the rest are passed as pointers into the function.

    func a_function (i64) -> (i64, f64, ptr) {...}

becomes

    i64 a_function (i64 _param_0, f64 *_return_1, ptr *_return_2) {...}

*/
static void emit_function_signature(FeFunction* f, StringBuilder* sb) {
    if (f->returns.len == 0) {
        sb_append_c(sb, "void ");
    } else {
        emit_type_name(f->params.at[0]->type, sb);
    }

    sb_append(sb, normalized_identifier(f->mod, f->sym, f->sym->name));

    sb_append_c(sb, "(");

    for_range(i, 0, f->params.len) {
        FeFunctionItem* item = f->params.at[i];

        emit_type_name(item->type, sb);

        sb_printf(sb, "_paramval_%d", i);
        if (i != f->params.len - 1 || f->returns.len > 1) {
            sb_printf(sb, ", ");
        }
    }
    for_range(i, 1, f->returns.len) {
        FeFunctionItem* item = f->returns.at[i];

        emit_type_ptr(item->type, sb);

        sb_printf(sb, "_returnval_%d", i);
        if (i != f->returns.len - 1) {
            sb_printf(sb, ", ");
        }
    }

    sb_append_c(sb, ")");
}

static void emit_symbol_defs(FeModule* m, StringBuilder* sb) {
    for_range(i, 0, m->symtab.len) {
        // printf("emitting symbol");
        FeSymbol* sym = m->symtab.at[i];

        if (sym->is_function) {
            emit_function_signature(sym->function, sb);
        } else {
            sb_append_c(sb, "extern ");
            TODO("");
        }
        sb_append_c(sb, ";\n");
    }

    sb_append_c(sb, "\n");
}

static const char* opstrings[] = {
    [FE_IR_SHL] = "<<",
    [FE_IR_ADD] = "+",
    [FE_IR_SUB] = "-",
    [FE_IR_IMUL] = "*",
    [FE_IR_UMUL] = "*",
};

static void emit_function(FeFunction* f, StringBuilder* sb) {
    emit_function_signature(f, sb);

    sb_append_c(sb, " {\n");

    // emit value predeclarations
    if (f->params.len != 0) {
        sb_append_c(sb, "        ");
        emit_type_name(f->params.at[0]->type, sb);
        sb_append_c(sb, "_returnval_0;\n");
    }
    foreach (FeBasicBlock* bb, f->blocks) {
        for_fe_ir(inst, *bb) {
            if (inst->type != FE_TYPE_VOID) {
                sb_append_c(sb, "        ");
                emit_type_name(inst->type, sb);
                sb_printf(sb, "_inst_%llx;\n", inst);
            }
        }
    }

    sb_printf(sb, "        goto _label_" str_fmt ";\n", str_arg(f->blocks.at[0]->name));

    // emit instructions
    foreach (FeBasicBlock* bb, f->blocks) {
        string label = normalized_identifier(f->mod, bb, bb->name);
        sb_printf(sb, "    _label_" str_fmt ":\n", str_arg(label));
        for_fe_ir(inst, *bb) {
            sb_append_c(sb, "\t");
            switch (inst->kind) {
            case FE_IR_PARAMVAL:
                sb_printf(sb, "_inst_%llx = _paramval_%d", inst, ((FeIrParamVal*)inst)->index);
                break;
            case FE_IR_CONST:
                switch (inst->type) {
                case FE_TYPE_I64: sb_printf(sb, "_inst_%llx = (i64) %lldll", inst, (i64)((FeIrConst*)inst)->i64); break;
                case FE_TYPE_I32: sb_printf(sb, "_inst_%llx = (i32) %lld", inst, (i64)((FeIrConst*)inst)->i32); break;
                case FE_TYPE_I16: sb_printf(sb, "_inst_%llx = (i16) %lld", inst, (i64)((FeIrConst*)inst)->i16); break;
                case FE_TYPE_I8: sb_printf(sb, "_inst_%llx = (i8)  %lld", inst, (i64)((FeIrConst*)inst)->i8); break;
                case FE_TYPE_F64: sb_printf(sb, "_inst_%llx = (f64) %lf", inst, (double)((FeIrConst*)inst)->f64); break;
                case FE_TYPE_F32: sb_printf(sb, "_inst_%llx = (f32) %lf", inst, (double)((FeIrConst*)inst)->f32); break;
                case FE_TYPE_F16: sb_printf(sb, "_inst_%llx = (f16) %lf", inst, (double)((FeIrConst*)inst)->f16); break;
                default: break;
                }
                break;
            case FE_IR_SHL:
            case FE_IR_ASR:
            case FE_IR_ADD:
            case FE_IR_IMUL:
                FeIrBinop* binop = (FeIrBinop*)inst;

                sb_printf(sb, "_inst_%llx = _inst_%llx %s _inst_%llx", inst, binop->lhs, opstrings[inst->kind], binop->rhs);
                break;
            case FE_IR_UMUL:
            case FE_IR_UDIV:
            case FE_IR_LSR:
                binop = (FeIrBinop*)inst;

                sb_printf(sb, "_inst_%llx = %s _inst_%llx %s %s _inst_%llx", inst, signed_to_unsigned_cast(binop->lhs->type), binop->lhs, opstrings[inst->kind], signed_to_unsigned_cast(binop->rhs->type), binop->rhs);
                break;
            case FE_IR_RETURN:
                TODO("redo this for the retval change");
                if (f->params.len > 0) {
                    sb_append_c(sb, "return _returnval_0");
                } else {
                    sb_append_c(sb, "return");
                }
            default:
                break;
            }
            sb_append_c(sb, ";\n");
        }
    }

    sb_append_c(sb, "}\n");
}

string fe_emit_c(FeModule* m) {
    StringBuilder sb = {0};
    sb_init(&sb);

    emit_prelude(m, &sb);

    emit_symbol_defs(m, &sb);

    for_urange(i, 0, m->functions_len) {
        FeFunction* f = m->functions[i];
        emit_function(f, &sb);
    }

    string out = string_alloc(sb_len(&sb));
    sb_write(&sb, out.raw);
    // printf("------------------- EMIT C -------------------\n");
    // printf(str_fmt, str_arg(out));

    sb_destroy(&sb);
    return out;
}
