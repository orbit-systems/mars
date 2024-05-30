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
    }

    return NULL_STR;
}

void air_emit_type_definitions(AtlasModule* am, StringBuilder* sb) {

    // hopefully this works lmao

    static char buffer[100];

    u32 count = 0;

    foreach(AIR_Type* t, am->ir_module->typegraph) {
        if (!is_null_str(simple_type_2_str(t))) continue;
        t->number = ++count;
    }

    foreach(AIR_Type* t, am->ir_module->typegraph) {
        if (!is_null_str(simple_type_2_str(t))) continue;

        memset(buffer, 0, 100);
        char* bufptr = buffer;
        
        bufptr += sprintf(bufptr, "type t%d = ", t->number);
        
        switch (t->kind) {
        case AIR_POINTER:
            if (is_null_str(simple_type_2_str(t->pointer))) {
                bufptr += sprintf(bufptr, "^t%d", t->pointer->number);
            } else {
                bufptr += sprintf(bufptr, "^"str_fmt, simple_type_2_str(t->pointer));
            }
            break;
        case AIR_ARRAY:
            if (is_null_str(simple_type_2_str(t->array.sub))) {
                bufptr += sprintf(bufptr, "[%lld]t%d", t->array.len, t->array.sub->number);
            } else {
                bufptr += sprintf(bufptr, "[%lld]"str_fmt, t->array.len, simple_type_2_str(t->array.sub));
            }
            break;
        default:
            CRASH("");
            break;
        }

        bufptr += sprintf(bufptr, "\n");

        sb_append_c(sb, bufptr); 
    }  
}