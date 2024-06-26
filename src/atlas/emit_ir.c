#include "atlas.h"
#include "strbuilder.h"
#include "ptrmap.h"

static string simple_type_2_str(AIR_Type* t) {
    switch (t->kind){
    case AIR_VOID: return constr("void");
    case AIR_BOOL: return constr("bool");
    case AIR_U8:   return constr("u8");
    case AIR_U16:  return constr("u16");
    case AIR_U32:  return constr("u32");
    case AIR_U64:  return constr("u64");
    case AIR_I8:   return constr("i8");
    case AIR_I16:  return constr("i16");
    case AIR_I32:  return constr("i32");
    case AIR_I64:  return constr("i64");
    case AIR_F16:  return constr("f16");
    case AIR_F32:  return constr("f32");
    case AIR_F64:  return constr("f64");
    case AIR_PTR:  return constr("ptr");
    }

    return NULL_STR;
}

static void emit_type_definitions(AtlasModule* am, StringBuilder* sb) {

    // hopefully this works lmao

    static char buffer[500];

    u32 count = 0;

    foreach(AIR_Type* t, am->ir_module->typegraph) {
        if (!is_null_str(simple_type_2_str(t))) continue;
        t->number = ++count;
    }

    foreach(AIR_Type* t, am->ir_module->typegraph) {
        if (!is_null_str(simple_type_2_str(t))) continue;

        memset(buffer, 0, sizeof(buffer));
        char* bufptr = buffer;
        
        bufptr += sprintf(bufptr, "type t%d = ", t->number);
        
        switch (t->kind) {
        case AIR_ARRAY:
            if (is_null_str(simple_type_2_str(t->array.sub))) {
                bufptr += sprintf(bufptr, "[%lld]t%d", t->array.len, t->array.sub->number);
            } else {
                bufptr += sprintf(bufptr, "[%lld]"str_fmt, t->array.len, str_arg(simple_type_2_str(t->array.sub)));
            }
            break;
        case AIR_AGGREGATE:

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

static int print_type_name(AIR_Type* t, char** bufptr) {
    char* og = *bufptr;
    if (is_null_str(simple_type_2_str(t))) {
        *bufptr += sprintf(*bufptr, "t%d", t->number);
    } else {
        *bufptr += sprintf(*bufptr, str_fmt, str_arg(simple_type_2_str(t)));
    }
    return *bufptr - og;
}

static int sb_type_name(AIR_Type* t, StringBuilder* sb) {
    char buf[16] = {0};

    if (is_null_str(simple_type_2_str(t))) {
        sprintf(buf, "t%d", t->number);
    } else {
        sprintf(buf, str_fmt, str_arg(simple_type_2_str(t)));
    }
    sb_append_c(sb, buf);
    return strlen(buf);
}

static u64 stack_object_index(AIR_Function* f, AIR_StackObject* obj) {
    for_urange(i, 0, f->stack.len) {
        AIR_StackObject* so = f->stack.at[i];
        if (so == obj) return i;
    }
    return UINT64_MAX;
}

static void emit_function(AIR_Function* f, StringBuilder* sb) {
    sb_append_c(sb, "func \"");
    sb_append(sb, f->sym->name);
    sb_append_c(sb, "\" (");

    for_range(i, 0, f->params_len) {
        sb_type_name(f->params[i]->T, sb);
        if (i + 1 != f->params_len) {
            sb_append_c(sb, ", ");
        }
    }

    sb_append_c(sb, ") -> (");

    for_range(i, 0, f->returns_len) {
        sb_type_name(f->returns[i]->T, sb);
        if (i + 1 != f->returns_len) {
            sb_append_c(sb, ", ");
        }
    }

    sb_append_c(sb, ") {\n");


    PtrMap ir2num = {0};
    ptrmap_init(&ir2num, 128);

    size_t counter = 1;

    for_urange(i, 0, f->stack.len) {
        AIR_StackObject* so = f->stack.at[i];
        sb_printf(sb, "    #%llu = stack.", i + 1);
        sb_type_name(so->t, sb);
        sb_append_c(sb, "\n");
        counter++;
    }

    for_urange(b, 0, f->blocks.len) {
        AIR_BasicBlock* bb = f->blocks.at[b];
        for_urange(i, 0, bb->len) {
            AIR* inst = bb->at[i];
            if (inst->tag == AIR_ELIMINATED) continue;
            inst->number = counter++; // assign a number to every IR
        }
    }

    for_urange(b, 0, f->blocks.len) {
        AIR_BasicBlock* bb = f->blocks.at[b];
        sb_append_c(sb, "  ");
        
        if (b == f->entry_idx) {
            sb_append_c(sb, "@entry ");
        }

        sb_append(sb, bb->name);
        sb_append_c(sb, ":\n");

        for_urange(i, 0, bb->len) {
            AIR* inst = bb->at[i];

            if (inst->tag == AIR_ELIMINATED) continue;

            sb_printf(sb, "    #%llu = ", inst->number);
            switch (inst->tag) {
            case AIR_ADD: {
                AIR_BinOp* binop = (AIR_BinOp*) inst;
                sb_append_c(sb, "add.");
                sb_type_name(binop->base.T, sb);
                sb_printf(sb, " #%llu, #%llu", binop->lhs->number, binop->rhs->number);
            } break;
            case AIR_LOAD: {
                AIR_Load* load = (AIR_Load*) inst;
                sb_append_c(sb, "load.");
                sb_type_name(load->base.T, sb);
                sb_printf(sb, " #%llu", load->location->number);
            } break;
            case AIR_STORE: {
                AIR_Store* store = (AIR_Store*) inst;
                sb_append_c(sb, "store.");
                sb_type_name(store->value->T, sb);
                sb_printf(sb, " #%llu, #%llu", store->location->number, store->value->number);
            } break;
            case AIR_MOV: {
                AIR_Mov* mov = (AIR_Mov*) inst;
                sb_printf(sb, "mov #%llu", mov->source->number);
            } break;
            case AIR_CONST: {
                AIR_Const* con = (AIR_Const*) inst;
                sb_append_c(sb, "const.");
                sb_type_name(inst->T, sb);
                switch (con->base.T->kind) {
                case AIR_I8:  sb_printf(sb, " %lld", (i64)con->i8);  break;
                case AIR_I16: sb_printf(sb, " %lld", (i64)con->i16); break;
                case AIR_I32: sb_printf(sb, " %lld", (i64)con->i32); break;
                case AIR_I64: sb_printf(sb, " %lld", (i64)con->i64); break;
                case AIR_U8:  sb_printf(sb, " %llu", (u64)con->u8);  break;
                case AIR_U16: sb_printf(sb, " %llu", (u64)con->u16); break;
                case AIR_U32: sb_printf(sb, " %llu", (u64)con->u32); break;
                case AIR_U64: sb_printf(sb, " %llu", (u64)con->u64); break;
                case AIR_F16: sb_printf(sb, " %f",   (f32)con->f16); break;
                case AIR_F32: sb_printf(sb, " %f",   con->f32); break;
                case AIR_F64: sb_printf(sb, " %lf",  con->f64); break;
                default:
                    sb_append_c(sb, "???");
                    break;
                }
            } break;
            case AIR_PARAMVAL: {
                AIR_ParamVal* param = (AIR_ParamVal*) inst;
                sb_printf(sb, "paramval %llu", param->param_idx);
            } break;
            case AIR_RETURNVAL: {
                AIR_ReturnVal* ret = (AIR_ReturnVal*) inst;
                sb_printf(sb, "returnval %llu, #%llu", ret->return_idx, ret->source->number);
            } break;
            case AIR_STACKOFFSET: {
                AIR_StackOffset* so = (AIR_StackOffset*) inst;
                u64 index = stack_object_index(f, so->object);
                sb_printf(sb, "stackoffset #%llu", index + 1);
            } break;
            case AIR_RETURN:
                sb_append_c(sb, "return");
                break;
            default:
                sb_printf(sb, "UNKNOWN %llu", inst->tag);
                break;
            }

            sb_append_c(sb, "\n");
        }

    }

    sb_append_c(sb, "}\n");
}

string air_textual_emit(AtlasModule* am) {
    StringBuilder sb = {0};
    sb_init(&sb);

    emit_type_definitions(am, &sb);
    for_range(i, 0, am->ir_module->functions_len) {
        emit_function(am->ir_module->functions[i], &sb);
    }

    string out = string_alloc(sb_len(&sb));
    sb_write(&sb, out.raw);
    return out;
}