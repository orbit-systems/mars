#include "iron/iron.h"
#include "common/strbuilder.h"
#include "common/ptrmap.h"

static string simple_type_2_str(FeType* t) {
    switch (t->kind){
    case FE_VOID: return constr("void");
    case FE_BOOL: return constr("bool");
    case FE_I8:   return constr("i8");
    case FE_I16:  return constr("i16");
    case FE_I32:  return constr("i32");
    case FE_I64:  return constr("i64");
    case FE_F16:  return constr("f16");
    case FE_F32:  return constr("f32");
    case FE_F64:  return constr("f64");
    case FE_PTR:  return constr("ptr");
    }

    return NULL_STR;
}

static void emit_type_definitions(FeModule* m, StringBuilder* sb) {

    // hopefully this works lmao

    static char buffer[500];

    u32 count = 0;

    foreach(FeType* t, m->typegraph) {
        if (!is_null_str(simple_type_2_str(t))) continue;
        t->number = ++count;
    }

    foreach(FeType* t, m->typegraph) {
        if (!is_null_str(simple_type_2_str(t))) continue;

        memset(buffer, 0, sizeof(buffer));
        char* bufptr = buffer;

        bufptr += sprintf(bufptr, "type t%d = ", t->number);

        switch (t->kind) {
        case FE_ARRAY:
            if (is_null_str(simple_type_2_str(t->array.sub))) {
                bufptr += sprintf(bufptr, "[%lld]t%d", t->array.len, t->array.sub->number);
            } else {
                bufptr += sprintf(bufptr, "[%lld]"str_fmt, t->array.len, str_arg(simple_type_2_str(t->array.sub)));
            }
            break;
        case FE_AGGREGATE:

            bufptr += sprintf(bufptr, "{");
            for_urange(i, 0, t->aggregate.len) {

                if (is_null_str(simple_type_2_str(t->array.sub))) {
                    bufptr += sprintf(bufptr, "t%d", t->aggregate.fields[i]->number);
                } else {
                    bufptr += sprintf(bufptr, str_fmt, str_arg(simple_type_2_str(t->aggregate.fields[i])));
                }

                if (i + 1 != t->aggregate.len) {
                    bufptr += sprintf(bufptr, ", ");
                }
            }
            bufptr += sprintf(bufptr, "}");
            break;
        default:
            CRASH("");
            break;
        }

        bufptr += sprintf(bufptr, "\n");

        sb_append_c(sb, bufptr);
    }
}

static int print_type_nme(FeType* t, char** bufptr) {
    char* og = *bufptr;
    if (is_null_str(simple_type_2_str(t))) {
        *bufptr += sprintf(*bufptr, "t%d", t->number);
    } else {
        *bufptr += sprintf(*bufptr, str_fmt, str_arg(simple_type_2_str(t)));
    }
    return *bufptr - og;
}

static int sb_type_name(FeType* t, StringBuilder* sb) {
    char buf[16] = {0};

    if (is_null_str(simple_type_2_str(t))) {
        sprintf(buf, "t%d", t->number);
    } else {
        sprintf(buf, str_fmt, str_arg(simple_type_2_str(t)));
    }
    sb_append_c(sb, buf);
    return strlen(buf);
}

static u64 stack_object_index(FeFunction* f, FeStackObject* obj) {
    for_urange(i, 0, f->stack.len) {
        FeStackObject* so = f->stack.at[i];
        if (so == obj) return i;
    }
    return UINT64_MAX;
}

static void emit_function(FeFunction* f, StringBuilder* sb) {
    sb_append_c(sb, "func ");
    sb_append(sb, f->sym->name);
    sb_append_c(sb, " (");

    for_range(i, 0, f->params_len) {
        sb_type_name(f->params[i]->type, sb);
        if (i + 1 != f->params_len) {
            sb_append_c(sb, ", ");
        }
    }

    sb_append_c(sb, ") -> (");

    for_range(i, 0, f->returns_len) {
        sb_type_name(f->returns[i]->type, sb);
        if (i + 1 != f->returns_len) {
            sb_append_c(sb, ", ");
        }
    }

    sb_append_c(sb, ") {\n");


    PtrMap ir2num = {0};
    ptrmap_init(&ir2num, 128);

    size_t counter = 1;

    for_urange(i, 0, f->stack.len) {
        FeStackObject* so = f->stack.at[i];
        sb_printf(sb, "    #%llu = stack.", i + 1);
        sb_type_name(so->t, sb);
        sb_append_c(sb, "\n");
        counter++;
    }

    for_urange(b, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[b];
        for_urange(i, 0, bb->len) {
            FeInst* inst = bb->at[i];
            if (inst->kind == FE_INST_ELIMINATED) continue;
            inst->number = counter++; // assign a number to every IR
        }
    }

    static char* opname[] = {
        [FE_INST_ADD]   = "add.",
        [FE_INST_SUB]   = "sub.",
        [FE_INST_IMUL]  = "imul.",
        [FE_INST_UMUL]  = "umul.",
        [FE_INST_IDIV]  = "idiv.",
        [FE_INST_UDIV]  = "udiv.",
        [FE_INST_AND]   = "and.",
        [FE_INST_OR]    = "or.",
        [FE_INST_XOR]   = "xor.",
        [FE_INST_SHL]   = "shl.",
        [FE_INST_LSR]   = "lsr.",
        [FE_INST_ASR]   = "asr.",
    };

    for_urange(b, 0, f->blocks.len) {
        FeBasicBlock* bb = f->blocks.at[b];
        sb_append_c(sb, "  ");

        if (b == f->entry_idx) {
            sb_append_c(sb, "@entry ");
        }

        sb_append(sb, bb->name);
        sb_append_c(sb, ":\n");

        for_urange(i, 0, bb->len) {
            FeInst* inst = bb->at[i];

            if (inst->kind == FE_INST_ELIMINATED) continue;

            sb_printf(sb, "    #%llu = ", inst->number);
            switch (inst->kind) {
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
            case FE_INST_ASR: {
                FeInstBinop* binop = (FeInstBinop*) inst;
                sb_append_c(sb, opname[inst->kind]);
                sb_type_name(binop->base.type, sb);
                sb_printf(sb, " #%llu, #%llu", binop->lhs->number, binop->rhs->number);
            } break;
            case FE_INST_LOAD: {
                FeInstLoad* load = (FeInstLoad*) inst;
                sb_append_c(sb, "load.");
                sb_type_name(load->base.type, sb);
                sb_printf(sb, " #%llu", load->location->number);
            } break;
            case FE_INST_STORE: {
                FeInstStore* store = (FeInstStore*) inst;
                sb_append_c(sb, "store.");
                sb_type_name(store->value->type, sb);
                sb_printf(sb, " #%llu, #%llu", store->location->number, store->value->number);
            } break;
            case FE_INST_MOV: {
                FeInstMov* mov = (FeInstMov*) inst;
                sb_printf(sb, "mov #%llu", mov->source->number);
            } break;
            case FE_INST_CONST: {
                FeInstLoadConst* con = (FeInstLoadConst*) inst;
                sb_append_c(sb, "const.");
                sb_type_name(inst->type, sb);
                switch (con->base.type->kind) {
                case FE_I8:  sb_printf(sb, " %lld", (i64)con->i8);  break;
                case FE_I16: sb_printf(sb, " %lld", (i64)con->i16); break;
                case FE_I32: sb_printf(sb, " %lld", (i64)con->i32); break;
                case FE_I64: sb_printf(sb, " %lld", (i64)con->i64); break;
                case FE_F16: sb_printf(sb, " %f",   (f32)con->f16); break;
                case FE_F32: sb_printf(sb, " %f",   con->f32); break;
                case FE_F64: sb_printf(sb, " %lf",  con->f64); break;
                default:
                    break;
                }
            } break;
            case FE_INST_PARAMVAL: {
                FeInstParamVal* param = (FeInstParamVal*) inst;
                sb_printf(sb, "paramval %llu", param->param_idx);
            } break;
            case FE_INST_RETURNVAL: {
                FeInstReturnVal* ret = (FeInstReturnVal*) inst;
                sb_printf(sb, "returnval %llu, #%llu", ret->return_idx, ret->source->number);
            } break;
            case FE_INST_STACKADDR: {
                FeInstStackAddr* so = (FeInstStackAddr*) inst;
                u64 index = stack_object_index(f, so->object);
                sb_printf(sb, "stackaddr #%llu", index + 1);
            } break;
            case FE_INST_RETURN:
                sb_append_c(sb, "return");
                break;
            default:
                sb_printf(sb, "UNKNOWN %llu", inst->kind);
                break;
            }

            sb_append_c(sb, "\n");
        }

    }

    sb_append_c(sb, "}\n");
}

string fe_emit_textual_ir(FeModule* m) {
    StringBuilder sb = {0};
    sb_init(&sb);

    emit_type_definitions(m, &sb);
    for_range(i, 0, m->functions_len) {
        emit_function(m->functions[i], &sb);
    }

    string out = string_alloc(sb_len(&sb));
    sb_write(&sb, out.raw);
    return out;
}
