#include "type.h"

void air_typegraph_init(AtlasModule* m) {
    da_init(&m->ir_module->typegraph, 16);
    m->ir_module->typegraph.alloca = arena_make(0x400);

    AIR_Type* none = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    none->kind = AIR_VOID;
    none->size = none->align = 0;
    da_append(&m->ir_module->typegraph, none);

    AIR_Type* bool = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    bool->kind = AIR_BOOL;
    bool->size = bool->align = 1;
    da_append(&m->ir_module->typegraph, bool);

    AIR_Type* u8 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    u8->kind = AIR_U8;
    u8->size = u8->align = 1;
    da_append(&m->ir_module->typegraph, u8);

    AIR_Type* u16 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    u16->kind = AIR_U16;
    u16->size = u16->align = 2;
    da_append(&m->ir_module->typegraph, u16);

    AIR_Type* u32 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    u32->kind = AIR_U32;
    u32->size = u32->align = 4;
    da_append(&m->ir_module->typegraph, u32);

    AIR_Type* u64 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    u64->kind = AIR_U64;
    u64->size = u64->align = 8;
    da_append(&m->ir_module->typegraph, u64);

    AIR_Type* i8 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    i8->kind = AIR_I8;
    i8->size = i8->align = 1;
    da_append(&m->ir_module->typegraph, i8);

    AIR_Type* i16 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    i16->kind = AIR_I16;
    i16->size = i16->align = 2;
    da_append(&m->ir_module->typegraph, i16);

    AIR_Type* i32 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    i32->kind = AIR_I32;
    i32->size = i32->align = 4;
    da_append(&m->ir_module->typegraph, i32);

    AIR_Type* i64 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    i64->kind = AIR_I64;
    i64->size = i64->align = 8;
    da_append(&m->ir_module->typegraph, i64);

    AIR_Type* f16 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    f16->kind = AIR_F16;
    f16->size = f16->align = 2;
    da_append(&m->ir_module->typegraph, f16);

    AIR_Type* f32 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    f32->kind = AIR_F32;
    f32->size = f32->align = 4;
    da_append(&m->ir_module->typegraph, f32);

    AIR_Type* f64 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(AIR_Type), alignof(AIR_Type));
    f64->kind = AIR_F64;
    f64->size = f64->align = 8;
    da_append(&m->ir_module->typegraph, f64);

    return;
}

AIR_Type* air_new_type(AtlasModule* m, u8 kind, u64 len) {

    u64 size = 0;
    switch (kind) {
    case AIR_VOID:
    case AIR_BOOL:
    case AIR_U8:
    case AIR_U16:
    case AIR_U32:
    case AIR_U64:
    case AIR_I8:
    case AIR_I16:
    case AIR_I32:
    case AIR_I64:
    case AIR_F16:
    case AIR_F32:
    case AIR_F64:
        return m->ir_module->typegraph.at[kind];
    case AIR_POINTER:
    case AIR_ARRAY:
        size = sizeof(AIR_Type);
        break;
    case AIR_AGGREGATE:
        size = sizeof(AIR_Type) + sizeof(AIR_Type*) * (len);
        break;
    default:
        UNREACHABLE;
    }

    AIR_Type* t = arena_alloc(&m->ir_module->typegraph.alloca, size, alignof(AIR_Type));

    *t = (AIR_Type){0};
    t->kind = kind;

    switch (kind) {
    case AIR_AGGREGATE:
        t->aggregate.len = len;
        break;
    case AIR_ARRAY:
        t->array.len = len;
        break;
    }

    return t;
}