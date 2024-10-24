#include "type.h"

// type engine

TypeGraph typegraph;

// #define LOG(...) printf(__VA_ARGS__)
#define LOG(...)

typedef struct {
    Type* dest;
    Type* src;
} type_pair;

da_typedef(type_pair);

// align a number (n) up to a power of two (align)
static u64 forceinline align_forward(u64 n, u64 align) {
    return (n + align - 1) & ~(align - 1);
}

void type_canonicalize_graph() {

    LOG("preliminary normalization\n");

    // preliminary normalization
    for_urange(i, 0, typegraph.len) {
        Type* t = typegraph.at[i];
        switch (t->tag) {
        case TYPE_ALIAS: // alias retargeting
            while (t->tag == TYPE_ALIAS) {
                merge_type_references(type_get_target(t), t, false);
                t = type_get_target(t);
            }
            break;
        case TYPE_ENUM: // variant sorting
            // using insertion sort for nice best-case complexity
            for_urange(i, 1, t->as_enum.variants.len) {
                u64 j = i;
                while (j > 0 && type_enum_variant_less(type_get_variant(t, j), type_get_variant(t, j - 1))) {
                    TypeEnumVariant temp = *type_get_variant(t, j);
                    t->as_enum.variants.at[j] = t->as_enum.variants.at[j - 1];
                    t->as_enum.variants.at[j - 1] = temp;
                    j--;
                }
            }
        default: break;
        }
    }

    LOG("preliminary normalization done\n");

    da(type_pair) equalities;
    da_init(&equalities, 1);

    // u64 num_of_types = 0;
    bool keep_going = true;
    while (keep_going) {
        // num_of_types = typegraph.len;
        keep_going = false;
        for_urange(i, TYPE_META_INTEGRAL, typegraph.len) {
            bool executed_TSA_at_all = false;
            if (typegraph.at[i]->tag == TYPE_ALIAS) continue;
            if (typegraph.at[i]->tag == TYPE_DISTINCT) continue;
            if (typegraph.at[i]->moved) continue;
            for_urange(j, i + 1, typegraph.len) {
                if (typegraph.at[j]->tag == TYPE_ALIAS) continue;
                if (typegraph.at[j]->tag == TYPE_DISTINCT) continue;
                if (typegraph.at[j]->moved) continue;
                // if (!(typegraph.at[i]->dirty || typegraph.at[j]->dirty)) continue;
                bool executed_TSA = false;
                LOG("are %d and %d equivalent?\n", i, j);
                if (type_equivalent(typegraph.at[i], typegraph.at[j], &executed_TSA)) {
                    LOG("yes!\n");
                    da_append(&equalities, ((type_pair){typegraph.at[i], typegraph.at[j]}));
                } else {
                    LOG("no!\n");
                }
                // if (executed_TSA) {
                //     executed_TSA_at_all = true;
                //     type_reset_numbers(1);
                // }
            }
            // if (executed_TSA_at_all) {
            //     type_reset_numbers(0);
            // }
            LOG("compared all to %p (%4zu / %4zu)\n", typegraph.at[i], i + 1, typegraph.len);
            // typegraph.at[i]->dirty = false;
        }
        for (int i = equalities.len - 1; i >= 0; --i) {
            Type* src = equalities.at[i].src;
            while (src->moved) {
                src = src->moved;
            }
            Type* dest = equalities.at[i].dest;
            while (dest->moved) {
                dest = dest->moved;
            }
            if (src == dest) continue;
            merge_type_references(dest, src, true);
            equalities.at[i].src->moved = dest;
            // dest->dirty = true;
            keep_going = true;
            LOG("merged %p <- %p (%4zu / %4zu)\n", dest, src, equalities.len - i, equalities.len);
        }
        // LOG("equalities merged\n");
        da_clear(&equalities);

        // for_urange(i, 0, typegraph.len) {
        //     if (typegraph.at[i]->moved) {
        //         da_unordered_remove_at(&typegraph, i);
        //         i--;
        //     }
        // }

        // so we have to modify the type IN PLACE
        for_urange(i, 0, typegraph.len) {
            if (typegraph.at[i]->moved) {
                Type* t = typegraph.at[i];
                Type* dest = t->moved;
                *t = (Type){0};

                t->tag = TYPE_ALIAS;
                t->as_reference.subtype = dest;
            }
        }
    }

    da_destroy(&equalities);
}

bool type_equivalent(Type* a, Type* b, bool* executed_TSA) {

    while (a->tag == TYPE_ALIAS) a = type_get_target(a);
    while (b->tag == TYPE_ALIAS) b = type_get_target(b);

    // simple checks
    if (a == b) return true;
    if (a->tag != b->tag) return false;
    if (a->tag < TYPE_META_INTEGRAL) return true;

    // a little more complex
    switch (a->tag) {
    case TYPE_POINTER:
    case TYPE_SLICE:
        // printf("POINTER TO %d COMPARED TO POINTER TO %d\n", a->as_reference.subtype->tag, b->as_reference.subtype->tag);
        if (a->as_reference.mutable != b->as_reference.mutable) return false;
        // printf("WHAT %p\n", type_get_target(a) == type_get_target(b));
        if (type_get_target(a) == type_get_target(b)) return true;
        break;
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_UNTYPED_AGGREGATE:
        if (a->as_aggregate.fields.len != b->as_aggregate.fields.len) return false;
        bool subtype_equals = true;
        for_urange(i, 0, a->as_aggregate.fields.len) {
            if (type_get_field(a, i)->subtype != type_get_field(b, i)->subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case TYPE_FUNCTION:
        if (a->as_function.params.len != b->as_function.params.len) {
            return false;
        }
        if (a->as_function.returns.len != b->as_function.returns.len) {
            return false;
        }
        subtype_equals = true;
        for_urange(i, 0, a->as_function.params.len) {
            if (a->as_function.params.at[i] != b->as_function.params.at[i]) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        for_urange(i, 0, a->as_function.returns.len) {
            if (a->as_function.returns.at[i] != b->as_function.returns.at[i]) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case TYPE_ARRAY:
        if (a->as_array.len != b->as_array.len) return false;
        break;
    case TYPE_DISTINCT: // distinct types are VERY strict
        return a == b;
    default: break;
    }

    // total structure analysis

    type_reset_numbers(0);
    type_reset_numbers(1);

    if (executed_TSA) *executed_TSA = true;

    u64 a_numbers = 1;
    type_locally_number(a, &a_numbers, 0);

    u64 b_numbers = 1;
    type_locally_number(b, &b_numbers, 1);

    if (a_numbers != b_numbers) {
        return false;
    }

    for_urange(i, 1, a_numbers) {
        if (!type_element_equivalent(type_get_from_num(i, 0), type_get_from_num(i, 1), 0, 1)) {
            return false;
        }
    }
    return true;
}

bool type_element_equivalent(Type* a, Type* b, int num_set_a, int num_set_b) {
    if (a->tag != b->tag) return false;

    switch (a->tag) {
    case TYPE_NONE:
    case TYPE_I8:
    case TYPE_I16:
    case TYPE_I32:
    case TYPE_I64:
    case TYPE_U8:
    case TYPE_U16:
    case TYPE_U32:
    case TYPE_U64:
    case TYPE_F16:
    case TYPE_F32:
    case TYPE_F64:
        return true;
        // if (a->type_nums[num_set_a] == b->type_nums[num_set_b]) return true;
        break;

    case TYPE_ALIAS:
    case TYPE_DISTINCT:
        return a == b;

    case TYPE_ENUM:
        if (a->as_enum.variants.len != b->as_enum.variants.len) {
            return false;
        }
        if (a->as_enum.backing_type != b->as_enum.backing_type) {
            return false;
        }

        for_urange(i, 0, a->as_enum.variants.len) {
            if (type_get_variant(a, i)->enum_val != type_get_variant(b, i)->enum_val) {
                return false;
            }
            if (!string_eq(type_get_variant(a, i)->name, type_get_variant(b, i)->name)) {
                return false;
            }
        }
        break;
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_UNTYPED_AGGREGATE:
        if (a->as_aggregate.fields.len != b->as_aggregate.fields.len) {
            return false;
        }
        for_urange(i, 0, a->as_aggregate.fields.len) {
            if (!string_eq(type_get_field(a, i)->name, type_get_field(b, i)->name)) {
                return false;
            }
            if (type_get_field(a, i)->subtype->type_nums[num_set_a] != type_get_field(b, i)->subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        break;
    case TYPE_FUNCTION:
        if (a->as_function.params.len != b->as_function.params.len) {
            return false;
        }
        if (a->as_function.returns.len != b->as_function.returns.len) {
            return false;
        }
        for_urange(i, 0, a->as_function.params.len) {
            if (a->as_function.params.at[i]->type_nums[num_set_a] != b->as_function.params.at[i]->type_nums[num_set_b]) {
                return false;
            }
        }
        for_urange(i, 0, a->as_function.returns.len) {
            if (a->as_function.returns.at[i]->type_nums[num_set_a] != b->as_function.returns.at[i]->type_nums[num_set_b]) {
                return false;
            }
        }
        break;
    case TYPE_ARRAY:
        if (a->as_array.len != b->as_array.len) {
            return false;
        }
        if (a->as_array.subtype->type_nums[num_set_a] != b->as_array.subtype->type_nums[num_set_b]) {
            return false;
        }
        break;
    case TYPE_POINTER:
    case TYPE_SLICE:
        if (a->as_reference.mutable != b->as_reference.mutable) return false;
        if (type_get_target(a)->type_nums[num_set_a] != type_get_target(b)->type_nums[num_set_b]) {
            return false;
        }
        break;
    default:
        printf("encountered %d", a->tag);
        CRASH("unknown type kind encountered");
        break;
    }

    return true;
}

void merge_type_references(Type* dest, Type* src, bool disable) {
    u64 src_index = type_get_index(src);
    if (src_index == UINT32_MAX) {
        return;
    }

    for_urange(i, 0, typegraph.len) {
        Type* t = typegraph.at[i];
        switch (t->tag) {
        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_UNTYPED_AGGREGATE:
            for_urange(i, 0, t->as_aggregate.fields.len) {
                if (type_get_field(t, i)->subtype == src) {
                    type_get_field(t, i)->subtype = dest;
                    // t->dirty = true;
                }
            }
            break;
        case TYPE_FUNCTION:
            for_urange(i, 0, t->as_function.params.len) {
                if (t->as_function.params.at[i] == src) {
                    t->as_function.params.at[i] = dest;
                    // t->dirty = true;
                }
            }
            for_urange(i, 0, t->as_function.returns.len) {
                if (t->as_function.returns.at[i] == src) {
                    t->as_function.returns.at[i] = dest;
                    // t->dirty = true;
                }
            }
            break;
        case TYPE_POINTER:
        case TYPE_SLICE:
        case TYPE_ALIAS:
        case TYPE_DISTINCT:
            if (type_get_target(t) == src) {
                type_set_target(t, dest);
                // t->dirty = true;
            }
            break;
        case TYPE_ARRAY:
            if (t->as_array.subtype == src) {
                t->as_array.subtype = dest;
                // t->dirty = true;
            }
            break;
        default:
            break;
        }
    }

    // printf("--- %zu\n", src_index);
    if (disable) src->moved = dest;
    // dest->dirty = true;
}

void type_locally_number(Type* t, u64* number, int num_set) {
    t = type_unalias(t);
    if (t->type_nums[num_set] != 0) return;

    t->type_nums[num_set] = (*number)++;

    switch (t->tag) {
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_UNTYPED_AGGREGATE:
        for_urange(i, 0, t->as_aggregate.fields.len) {
            type_locally_number(type_get_field(t, i)->subtype, number, num_set);
        }
        break;
    case TYPE_FUNCTION:
        for_urange(i, 0, t->as_function.params.len) {
            type_locally_number(t->as_function.params.at[i], number, num_set);
        }
        for_urange(i, 0, t->as_function.returns.len) {
            type_locally_number(t->as_function.returns.at[i], number, num_set);
        }
        break;
    case TYPE_ARRAY:
        type_locally_number(t->as_array.subtype, number, num_set);
        break;
    case TYPE_POINTER:
    case TYPE_SLICE:
    case TYPE_DISTINCT:
        type_locally_number(type_get_target(t), number, num_set);
        break;
    default:
        break;
    }
}

void type_reset_numbers(int num_set) {
    for_urange(i, 0, typegraph.len) {
        typegraph.at[i]->type_nums[num_set] = 0;
    }
}

Type* type_get_from_num(u16 num, int num_set) {
    for_urange(i, 0, typegraph.len) {
        if (typegraph.at[i]->type_nums[num_set] == num) return typegraph.at[i];
    }
    return NULL;
}

Type* make_type(u8 tag) {
    if (typegraph.at == NULL) {
        make_type_graph();
    }
    if (tag < TYPE_META_INTEGRAL && (typegraph.len >= TYPE_META_INTEGRAL)) {
        return typegraph.at[tag];
    }

    Type* t = mars_alloc(sizeof(Type));
    *t = (Type){0};
    t->moved = false;
    t->tag = tag;

    switch (tag) {
    case TYPE_STRUCT:
    case TYPE_UNION:
    case TYPE_UNTYPED_AGGREGATE:
        da_init(&t->as_aggregate.fields, 4);
        break;
    case TYPE_ENUM:
        da_init(&t->as_enum.variants, 4);
        t->as_enum.backing_type = typegraph.at[TYPE_I64];
        break;
    case TYPE_FUNCTION:
        da_init(&t->as_function.params, 4);
        da_init(&t->as_function.returns, 1);
        break;
    default: break;
    }
    // t->dirty = true;
    t->size = UINT32_MAX;
    t->align = UINT32_MAX;
    da_append(&typegraph, t);
    return t;
}

void make_type_graph() {
    typegraph = (TypeGraph){0};
    da_init(&typegraph, 3);

    for_range(i, 0, TYPE_META_INTEGRAL) {
        make_type(i);
    }
}

forceinline void type_add_param(Type* s, Type* sub) {
    da_append(&s->as_function.params, sub);
}

forceinline Type* type_get_param(Type* s, size_t i) {
    return s->as_function.params.at[i];
}

forceinline void type_add_return(Type* s, Type* sub) {
    da_append(&s->as_function.returns, sub);
}

forceinline Type* type_get_return(Type* s, size_t i) {
    return s->as_function.returns.at[i];
}

forceinline void type_add_field(Type* s, string name, Type* sub) {
    da_append(&s->as_aggregate.fields, ((TypeStructField){name, sub}));
}

forceinline TypeStructField* type_get_field(Type* s, size_t i) {
    return &s->as_aggregate.fields.at[i];
}

forceinline void type_add_variant(Type* e, string name, i64 val) {
    da_append(&e->as_enum.variants, ((TypeEnumVariant){name, val}));
}

forceinline TypeEnumVariant* type_get_variant(Type* e, size_t i) {
    return &e->as_enum.variants.at[i];
}

forceinline bool type_enum_variant_less(TypeEnumVariant* a, TypeEnumVariant* b) {
    if (a->enum_val == b->enum_val) {
        // use string names
        return string_cmp(a->name, b->name) < 0;
    } else {
        return a->enum_val < b->enum_val;
    }
}

forceinline void type_set_target(Type* p, Type* dest) {
    p->as_reference.subtype = dest;
}

forceinline Type* type_get_target(Type* p) {
    return p->as_reference.subtype;
}

u64 type_get_index(Type* t) {
    for_urange(i, 0, typegraph.len) {
        if (typegraph.at[i] == t) return i;
    }
    return UINT32_MAX;
}

void print_type_graph() {
    printf("-------------------------\n");
    for_urange(i, 0, typegraph.len) {
        Type* t = typegraph.at[i];
        if (t->moved) continue;
        // printf("%-2zu   [%-2hu, %-2hu]\t", i, t->type_nums[0], t->type_nums[1]);
        printf("%-2zu\t", i);
        // printf(t->dirty ? "[dirty]\t" : "[clean]\t");
        switch (t->tag) {
        case TYPE_NONE: printf("(none)\n"); break;
        case TYPE_UNTYPED_INT: printf("untyped int\n"); break;
        case TYPE_UNTYPED_FLOAT: printf("untyped float\n"); break;
        case TYPE_I8: printf("i8\n"); break;
        case TYPE_I16: printf("i16\n"); break;
        case TYPE_I32: printf("i32\n"); break;
        case TYPE_I64: printf("i64\n"); break;
        case TYPE_U8: printf("u8\n"); break;
        case TYPE_U16: printf("u16\n"); break;
        case TYPE_U32: printf("u32\n"); break;
        case TYPE_U64: printf("u64\n"); break;
        case TYPE_F16: printf("f16\n"); break;
        case TYPE_F32: printf("f32\n"); break;
        case TYPE_F64: printf("f64\n"); break;
        case TYPE_BOOL: printf("bool\n"); break;
        case TYPE_ALIAS:
            printf("(%zu)\n", type_get_index(type_get_target(t)));
            break;
        case TYPE_DISTINCT:
            printf("distinct %zu\n", type_get_index(type_get_target(t)));
            break;
        case TYPE_POINTER:
            printf("^%s %zu\n", t->as_reference.mutable ? "mut" : "let", type_get_index(type_get_target(t)));
            break;
        case TYPE_SLICE:
            printf("slice %zu\n", type_get_index(type_get_target(t)));
            break;
        case TYPE_ARRAY:
            printf("array %zu\n", type_get_index(t->as_array.subtype));
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
            printf(t->tag == TYPE_STRUCT ? "struct\n" : "union\n");
            for_urange(field, 0, t->as_aggregate.fields.len) {
                printf("\t\t.%s : %zu\n", type_get_field(t, field)->name, type_get_index(type_get_field(t, field)->subtype));
            }
            break;
        case TYPE_FUNCTION:
            printf("function (\n");
            for_urange(field, 0, t->as_function.params.len) {
                printf("\t\t%zu\n", type_get_index(type_get_param(t, field)));
            }
            printf("\t) -> (\n");
            for_urange(field, 0, t->as_function.returns.len) {
                printf("\t\t%zu\n", type_get_index(type_get_return(t, field)));
            }
            printf("\t)\n");

        default:
            printf("unknown tag %d\n", t->tag);
            break;
        }
    }
}

// is type unboundedly recursive (have infinite size)?
bool type_is_infinite(Type* t) {
    if (t->visited) return true;

    t->visited = true;
    bool is_inf = false;

    switch (t->tag) {
    case TYPE_STRUCT:
    case TYPE_UNION:
        for_urange(i, 0, t->as_aggregate.fields.len) {
            if (type_is_infinite(type_get_field(t, i)->subtype)) {
                is_inf = true;
                break;
            }
        }
        break;
    case TYPE_FUNCTION:
        for_urange(i, 0, t->as_function.params.len) {
            if (type_is_infinite(t->as_function.params.at[i])) {
                is_inf = true;
                break;
            }
        }
        for_urange(i, 0, t->as_function.returns.len) {
            if (type_is_infinite(t->as_function.returns.at[i])) {
                is_inf = true;
                break;
            }
        }
        break;
    case TYPE_ARRAY:
        is_inf = type_is_infinite(t->as_array.subtype);
        break;
    case TYPE_POINTER:
    case TYPE_SLICE:
        break;

    case TYPE_DISTINCT:
    case TYPE_ALIAS:
        type_is_infinite(type_get_target(t));
        break;
    default:
        break;
    }

    t->visited = false;
    return is_inf;
}

u32 static size_of_internal(Type* t) {
    if (t->size != UINT32_MAX) return t->size;

    switch (t->tag) {
    case TYPE_NONE:
        return t->size = 0;
    case TYPE_I8:
    case TYPE_U8:
    case TYPE_BOOL:
        return t->size = 1;
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_F16:
        return t->size = 2;
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_F32:
        return t->size = 4;
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_F64:
    case TYPE_POINTER:
    case TYPE_FUNCTION: // remember, its a function POINTER!
        return t->size = 8;
    case TYPE_SLICE:
        return t->size = 16;
    case TYPE_ENUM:
        return t->size = size_of_internal(t->as_enum.backing_type);
    case TYPE_ALIAS:
    case TYPE_DISTINCT:
        return t->size = size_of_internal(t->as_reference.subtype);
    case TYPE_ARRAY:
        return t->size = size_of_internal(t->as_array.subtype) * t->as_array.len;
    case TYPE_UNION: {
        u64 max_size = 0;
        for_urange(i, 0, t->as_aggregate.fields.len) {
            u64 size = size_of_internal(type_get_field(t, i)->subtype);
            if (size > max_size) max_size = size;
        }
        return align_forward(max_size, type_real_align_of(t));
    } break;
    case TYPE_STRUCT: {
        u64 full_size = 0;
        for_urange(i, 0, t->as_aggregate.fields.len) {
            type_get_field(t, i)->offset = full_size;
            u64 elem_size = size_of_internal(type_get_field(t, i)->subtype);
            full_size += elem_size;
        }
        return align_forward(full_size, type_real_align_of(t));
    } break;
    default:
        CRASH("unreachable");
    }
}

u32 type_real_size_of(Type* t) {
    if (type_is_infinite(t)) return UINT32_MAX;
    return size_of_internal(t);
}

u32 static align_of_internal(Type* t) {
    if (t->align != UINT32_MAX) return t->align;

    switch (t->tag) {
    case TYPE_NONE:
        return t->align = 0;
    case TYPE_I8:
    case TYPE_U8:
        return t->align = 1;
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_F16:
        return t->align = 2;
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_F32:
        return t->align = 4;
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_F64:
    case TYPE_POINTER:
    case TYPE_FUNCTION: // remember, its a function POINTER!
    case TYPE_SLICE:
        return t->align = 8;
    case TYPE_ENUM:
        return t->align = align_of_internal(t->as_enum.backing_type);
    case TYPE_ALIAS:
    case TYPE_DISTINCT:
        return t->align = align_of_internal(t->as_reference.subtype);
    case TYPE_ARRAY:
        return t->align = align_of_internal(t->as_array.subtype);

    case TYPE_UNION:
    case TYPE_STRUCT: {
        u64 max_align = 0;
        for_urange(i, 0, t->as_aggregate.fields.len) {
            u64 align = align_of_internal(type_get_field(t, i)->subtype);
            if (align > max_align) max_align = align;
        }
        return t->align = max_align;
    } break;
    default:
        CRASH("unreachable");
    }
}

u32 type_real_align_of(Type* t) {
    if (type_is_infinite(t)) return UINT32_MAX;
    return align_of_internal(t);
}

forceinline bool is_raw_pointer(Type* p) {
    return (p->tag == TYPE_POINTER && p->as_reference.subtype->tag == TYPE_NONE);
}

Type* type_unalias(Type* t) {
    while (t->tag == TYPE_ALIAS) {
        t = t->as_reference.subtype;
    }
    return t;
}