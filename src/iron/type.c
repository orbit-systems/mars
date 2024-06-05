#include "iron.h"

void fe_typegraph_init(FeModule* m) {

    da_init(&m->ir_module->typegraph, 16);
    m->ir_module->typegraph.alloca = arena_make(0x400);

    FeType* none = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    none->kind = FE_VOID;
    // none->size = none->align = 0;
    da_append(&m->ir_module->typegraph, none);

    FeType* bool = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    bool->kind = FE_BOOL;
    // bool->size = bool->align = 1;
    da_append(&m->ir_module->typegraph, bool);

    FeType* ptr = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    ptr->kind = FE_PTR;
    // f64->size = f64->align = 8;
    da_append(&m->ir_module->typegraph, ptr);

    FeType* u8 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    u8->kind = FE_U8;
    // u8->size = u8->align = 1;
    da_append(&m->ir_module->typegraph, u8);

    FeType* u16 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    u16->kind = FE_U16;
    // u16->size = u16->align = 2;
    da_append(&m->ir_module->typegraph, u16);

    FeType* u32 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    u32->kind = FE_U32;
    // u32->size = u32->align = 4;
    da_append(&m->ir_module->typegraph, u32);

    FeType* u64 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    u64->kind = FE_U64;
    // u64->size = u64->align = 8;
    da_append(&m->ir_module->typegraph, u64);

    FeType* i8 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i8->kind = FE_I8;
    // i8->size = i8->align = 1;
    da_append(&m->ir_module->typegraph, i8);

    FeType* i16 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i16->kind = FE_I16;
    // i16->size = i16->align = 2;
    da_append(&m->ir_module->typegraph, i16);

    FeType* i32 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i32->kind = FE_I32;
    // i32->size = i32->align = 4;
    da_append(&m->ir_module->typegraph, i32);

    FeType* i64 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i64->kind = FE_I64;
    // i64->size = i64->align = 8;
    da_append(&m->ir_module->typegraph, i64);

    FeType* f16 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    f16->kind = FE_F16;
    // f16->size = f16->align = 2;
    da_append(&m->ir_module->typegraph, f16);

    FeType* f32 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    f32->kind = FE_F32;
    // f32->size = f32->align = 4;
    da_append(&m->ir_module->typegraph, f32);

    FeType* f64 = arena_alloc(&m->ir_module->typegraph.alloca, sizeof(FeType), alignof(FeType));
    f64->kind = FE_F64;
    // f64->size = f64->align = 8;
    da_append(&m->ir_module->typegraph, f64);

    return;
}

FeType* fe_type(FeModule* m, u8 kind, u64 len) {

    u64 size = 0;
    switch (kind) {
    case FE_VOID:
    case FE_BOOL:
    case FE_PTR:
    case FE_U8:
    case FE_U16:
    case FE_U32:
    case FE_U64:
    case FE_I8:
    case FE_I16:
    case FE_I32:
    case FE_I64:
    case FE_F16:
    case FE_F32:
    case FE_F64:
        return m->ir_module->typegraph.at[kind];
    case FE_ARRAY:
        size = sizeof(FeType);
        break;
    case FE_AGGREGATE:
        size = sizeof(FeType) + sizeof(FeType*) * (len);
        break;
    default:
        UNREACHABLE;
    }

    FeType* t = arena_alloc(&m->ir_module->typegraph.alloca, size, alignof(FeType));

    *t = (FeType){0};
    t->kind = kind;

    switch (kind) {
    case FE_AGGREGATE:
        t->aggregate.len = len;
        break;
    case FE_ARRAY:
        t->array.len = len;
        break;
    }

    return t;
}