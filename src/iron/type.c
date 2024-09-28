#include "iron/iron.h"

void fe_typegraph_init(FeModule* m) {

    da_init(&m->typegraph, 16);
    m->typegraph.alloca = arena_make(0x1000);

    return;
}

bool fe_type_has_equivalence(FeType t) {
    switch (t) {
    case FE_TYPE_BOOL:
    case FE_TYPE_PTR:
    case FE_TYPE_I8:
    case FE_TYPE_I16:
    case FE_TYPE_I32:
    case FE_TYPE_I64:
    case FE_TYPE_F16:
    case FE_TYPE_F32:
    case FE_TYPE_F64:
        return true;
    default:
        return false;
    }
}

bool fe_type_is_scalar(FeType t) {
    switch (t) {
    case FE_TYPE_BOOL:
    case FE_TYPE_PTR:
    case FE_TYPE_I8:
    case FE_TYPE_I16:
    case FE_TYPE_I32:
    case FE_TYPE_I64:
    case FE_TYPE_F16:
    case FE_TYPE_F32:
    case FE_TYPE_F64:
        return true;
    default:
        return false;
    }
}

bool fe_type_has_ordering(FeType t) {
    switch (t) {
    case FE_TYPE_I8:
    case FE_TYPE_I16:
    case FE_TYPE_I32:
    case FE_TYPE_I64:
    case FE_TYPE_F16:
    case FE_TYPE_F32:
    case FE_TYPE_F64:
        return true;
    default:
        return false;
    }
}

bool fe_type_is_integer(FeType t) {
    switch (t) {
    case FE_TYPE_I8:
    case FE_TYPE_I16:
    case FE_TYPE_I32:
    case FE_TYPE_I64:
        return true;
    default:
        return false;
    }
}

bool fe_type_is_float(FeType t) {
    switch (t) {
    case FE_TYPE_F16:
    case FE_TYPE_F32:
    case FE_TYPE_F64:
        return true;
    default:
        return false;
    }
}


FeType fe_type_array(FeModule* m, FeType subtype, u64 len) {

    FeAggregateType* t = arena_alloc(&m->typegraph.alloca, sizeof(FeAggregateType), alignof(FeAggregateType));

    da_append(&m->typegraph, t);
    *t = (FeAggregateType){0};
    t->kind = FE_TYPE_ARRAY;
    t->array.sub = subtype;
    t->array.len = len;

    return m->typegraph.len - 1 + _FE_TYPE_SIMPLE_END;
}

FeType fe_type_record(FeModule* m, u64 len) {

    FeAggregateType* t = arena_alloc(&m->typegraph.alloca, 
        sizeof(FeAggregateType) + sizeof(FeType) * (len), 
        alignof(FeAggregateType)
    );
    da_append(&m->typegraph, t);

    *t = (FeAggregateType){0};
    t->kind = FE_TYPE_RECORD;
    t->record.len = len;

    return m->typegraph.len - 1 + _FE_TYPE_SIMPLE_END;
}

// return null if passed a simple type
FeAggregateType* fe_type_get_structure(FeModule* m, FeType t) {
    if (t < _FE_TYPE_SIMPLE_END) return NULL;
    return m->typegraph.at[t - _FE_TYPE_SIMPLE_END];
}