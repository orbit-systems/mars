#include "iron/iron.h"

void fe_typegraph_init(FeModule* m) {

    da_init(&m->typegraph, 16);
    m->typegraph.alloca = arena_make(0x1000);

    FeType* none = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    none->kind = FE_TYPE_VOID;
    // none->size = none->align = 0;
    da_append(&m->typegraph, none);

    FeType* bool = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    bool->kind = FE_TYPE_BOOL;
    // bool->size = bool->align = 1;
    da_append(&m->typegraph, bool);

    FeType* ptr = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    ptr->kind = FE_TYPE_PTR;
    // f64->size = f64->align = 8;
    da_append(&m->typegraph, ptr);

    FeType* i8 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i8->kind = FE_TYPE_I8;
    // i8->size = i8->align = 1;
    da_append(&m->typegraph, i8);

    FeType* i16 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i16->kind = FE_TYPE_I16;
    // i16->size = i16->align = 2;
    da_append(&m->typegraph, i16);

    FeType* i32 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i32->kind = FE_TYPE_I32;
    // i32->size = i32->align = 4;
    da_append(&m->typegraph, i32);

    FeType* i64 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    i64->kind = FE_TYPE_I64;
    // i64->size = i64->align = 8;
    da_append(&m->typegraph, i64);

    FeType* f16 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    f16->kind = FE_TYPE_F16;
    // f16->size = f16->align = 2;
    da_append(&m->typegraph, f16);

    FeType* f32 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    f32->kind = FE_TYPE_F32;
    // f32->size = f32->align = 4;
    da_append(&m->typegraph, f32);

    FeType* f64 = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));
    f64->kind = FE_TYPE_F64;
    // f64->size = f64->align = 8;
    da_append(&m->typegraph, f64);

    return;
}

FeType* fe_type(FeModule* m, u8 kind) {

    u64 size = 0;
    switch (kind) {
    case FE_TYPE_VOID:
    case FE_TYPE_BOOL:
    case FE_TYPE_PTR:
    case FE_TYPE_I8:
    case FE_TYPE_I16:
    case FE_TYPE_I32:
    case FE_TYPE_I64:
    case FE_TYPE_F16:
    case FE_TYPE_F32:
    case FE_TYPE_F64:
        return m->typegraph.at[kind - 1];
    case FE_TYPE_ARRAY:
        // size = sizeof(FeType);
        // break;
        CRASH("use fe_type_array() to create an array type");
    case FE_TYPE_RECORD:
        // size = sizeof(FeType) + sizeof(FeType*) * (len);
        CRASH("use fe_type_aggregate() to create an aggregate type");
        break;
    default:
        UNREACHABLE;
    }
}

FeType* fe_type_array(FeModule* m, FeType* subtype, u64 len) {

    FeType* t = arena_alloc(&m->typegraph.alloca, sizeof(FeType), alignof(FeType));

    *t = (FeType){0};
    t->kind = FE_TYPE_ARRAY;
    t->array.sub = subtype;
    t->array.len = len;

    return t;
}

FeType* fe_type_aggregate(FeModule* m, u64 len) {

    FeType* t = arena_alloc(&m->typegraph.alloca, 
        sizeof(FeType) + sizeof(FeType*) * (len), 
        alignof(FeType)
    );

    *t = (FeType){0};
    t->kind = FE_TYPE_RECORD;
    t->aggregate.len = len;

    return t;
}