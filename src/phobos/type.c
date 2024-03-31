#include "type.h"

#include <locale.h>

// type engine

type_graph typegraph;

// #define LOG(...) printf(__VA_ARGS__)
#define LOG(...)

typedef struct {
    type* dest;
    type* src;
} type_pair;

da_typedef(type_pair);

// align a number (n) up to a power of two (align)
static u64 forceinline align_forward(u64 n, u64 align) {
    return (n + align - 1) & ~(align - 1);
}

void canonicalize_type_graph() {

    // preliminary normalization
    FOR_URANGE(i, 0, typegraph.len) {
        type* t = typegraph.at[i];
        switch (t->tag) {
        case T_ALIAS: // alias retargeting
            while (t->tag == T_ALIAS) {
                merge_type_references(get_target(t), t, false);
                t = get_target(t);
            }
            break;
        case T_ENUM: // variant sorting
            // using insertion sort for nice best-case complexity
            FOR_URANGE(i, 1, t->as_enum.variants.len) {
                u64 j = i;
                while (j > 0 && type_enum_variant_less(get_variant(t, j), get_variant(t, j-1))) {
                    enum_variant temp = *get_variant(t, j);
                    t->as_enum.variants.at[j] = t->as_enum.variants.at[j-1];
                    t->as_enum.variants.at[j-1] = temp;
                    j--;
                }
            }
        default: break;
        }
    }

    da(type_pair) equalities;
    da_init(&equalities, 1);

    // u64 num_of_types = 0;
    bool keep_going = true;
    while (keep_going) {
        // num_of_types = typegraph.len;
        keep_going = false;
        FOR_URANGE(i, T_meta_INTEGRAL, typegraph.len) {
            bool executed_TSA_at_all = false;
            if (typegraph.at[i]->tag == T_ALIAS) continue;
            if (typegraph.at[i]->tag == T_DISTINCT) continue;
            if (typegraph.at[i]->moved) continue;
            FOR_URANGE(j, i+1, typegraph.len) {
                if (typegraph.at[j]->tag == T_ALIAS) continue;
                if (typegraph.at[j]->tag == T_DISTINCT) continue;
                if (typegraph.at[j]->moved) continue;
                // if (!(typegraph.at[i]->dirty || typegraph.at[j]->dirty)) continue;
                bool executed_TSA = false;
                if (types_are_equivalent(typegraph.at[i], typegraph.at[j], &executed_TSA)) {
                    da_append(&equalities, ((type_pair){typegraph.at[i], typegraph.at[j]}));
                }
                if (executed_TSA) {
                    executed_TSA_at_all = true;
                    type_reset_numbers(1);
                }
            }
            if (executed_TSA_at_all) {
                type_reset_numbers(0);
            }
            LOG("compared all to %p (%4zu / %4zu)\n", typegraph.at[i], i+1, typegraph.len);
            // typegraph.at[i]->dirty = false;
        }
        for (int i = equalities.len-1; i >= 0; --i) {
            type* src = equalities.at[i].src;
            while (src->moved) {
                src = src->moved; 
            }
            type* dest = equalities.at[i].dest;
            while (dest->moved) {
                dest = dest->moved; 
            }
            if (src == dest) continue;
            merge_type_references(dest, src, true);
            equalities.at[i].src->moved = dest;
            // dest->dirty = true;
            keep_going = true;
            LOG("merged %p <- %p (%4zu / %4zu)\n", dest, src, equalities.len-i, equalities.len);
            
        }
        // LOG("equalities merged\n");
        da_clear(&equalities);


        FOR_URANGE(i, 0, typegraph.len) {
            if (typegraph.at[i]->moved) {
                da_unordered_remove_at(&typegraph, i);
                i--;
            }
        }
    }

    da_destroy(&equalities);
}

bool types_are_equivalent(type* a, type* b, bool* executed_TSA) {

    while (a->tag == T_ALIAS) a = get_target(a);
    while (b->tag == T_ALIAS) b = get_target(b);

    // simple checks
    if (a == b) return true;
    if (a->tag != b->tag) return false;
    if (a->tag < T_meta_INTEGRAL) return true;
    
    // a little more complex
    switch (a->tag) {
    case T_POINTER:
    case T_SLICE:
        if (a->as_reference.mutable != b->as_reference.mutable) return false;
        if (get_target(a) == get_target(b)) return true;
        break;
    case T_STRUCT:
    case T_UNION:
    case T_UNTYPED_AGGREGATE:
        if (a->as_aggregate.fields.len != b->as_aggregate.fields.len) return false;
        bool subtype_equals = true;
        FOR_URANGE(i, 0, a->as_aggregate.fields.len) {
            if (get_field(a, i)->subtype != get_field(b, i)->subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case T_FUNCTION:
        if (a->as_function.params.len != b->as_function.params.len) {
            return false;
        }
        if (a->as_function.returns.len != b->as_function.returns.len) {
            return false;
        }
        subtype_equals = true;
        FOR_URANGE(i, 0, a->as_function.params.len) {
            if (a->as_function.params.at[i].subtype != b->as_function.params.at[i].subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        FOR_URANGE(i, 0, a->as_function.returns.len) {
            if (a->as_function.returns.at[i].subtype != b->as_function.returns.at[i].subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case T_ARRAY:
        if (a->as_array.len != b->as_array.len) return false;
        break;
    case T_DISTINCT: // distinct types are VERY strict
        return a == b;
    case T_ADDR:
        return a == b; // this should really not do anything
    default: break;
    }

    // total structure analysis

    *executed_TSA = true;

    u64 a_numbers = 1;
    type_locally_number(a, &a_numbers, 0);

    u64 b_numbers = 1;
    type_locally_number(b, &b_numbers, 1);

    if (a_numbers != b_numbers) {
        return false;
    }

    FOR_URANGE(i, 1, a_numbers) {
        if (!type_element_equivalent(get_type_from_num(i, 0), get_type_from_num(i, 1), 0, 1)) {
            return false;
        }
    }
    return true;
}

bool type_element_equivalent(type* a, type* b, int num_set_a, int num_set_b) {
    if (a->tag != b->tag) return false;

    switch (a->tag) {
    case T_NONE:
    case T_I8:  case T_I16: case T_I32: case T_I64:
    case T_U8:  case T_U16: case T_U32: case T_U64:
    case T_F16: case T_F32: case T_F64: case T_ADDR:
        return true;
        // if (a->type_nums[num_set_a] == b->type_nums[num_set_b]) return true;
        break;

    case T_ALIAS:
    case T_DISTINCT:
        return a == b;

    case T_ENUM:
        if (a->as_enum.variants.len != b->as_enum.variants.len) {
            return false;
        }
        if (a->as_enum.backing_type != b->as_enum.backing_type) {
            return false;
        }

        FOR_URANGE(i, 0, a->as_enum.variants.len) {
            if (get_variant(a, i)->enum_val != get_variant(b, i)->enum_val) {
                return false;
            }
            if (!string_eq(get_variant(a, i)->name, get_variant(b, i)->name)) {
                return false;
            }
        }
        break;
    case T_STRUCT:
    case T_UNION:
    case T_UNTYPED_AGGREGATE:
        if (a->as_aggregate.fields.len != b->as_aggregate.fields.len) {
            return false;
        }
        FOR_URANGE(i, 0, a->as_aggregate.fields.len) {
            if (!string_eq(get_field(a, i)->name, get_field(b, i)->name)) {
                return false;
            }
            if (get_field(a, i)->subtype->type_nums[num_set_a] != get_field(b, i)->subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        break;
    case T_FUNCTION:
        if (a->as_function.params.len != b->as_function.params.len) {
            return false;
        }
        if (a->as_function.returns.len != b->as_function.returns.len) {
            return false;
        }
        FOR_URANGE(i, 0, a->as_function.params.len) {
            if (!string_eq(a->as_function.params.at[i].name, b->as_function.params.at[i].name)) {
                return false;
            }
            if (a->as_function.params.at[i].subtype->type_nums[num_set_a] != b->as_function.params.at[i].subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        FOR_URANGE(i, 0, a->as_function.returns.len) {
            if (!string_eq(a->as_function.returns.at[i].name, b->as_function.returns.at[i].name)) {
                return false;
            }
            if (a->as_function.returns.at[i].subtype->type_nums[num_set_a] != b->as_function.returns.at[i].subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        break;
    case T_ARRAY:
        if (a->as_array.len != b->as_array.len) {
            return false;
        }
        if (a->as_array.subtype->type_nums[num_set_a] != b->as_array.subtype->type_nums[num_set_b]) {
            return false;
        }
        break;
    case T_POINTER:
    case T_SLICE:
        if (a->as_reference.mutable != b->as_reference.mutable) return false;
        if (get_target(a)->type_nums[num_set_a] != get_target(b)->type_nums[num_set_b]) {
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

void merge_type_references(type* dest, type* src, bool disable) {

    u64 src_index = get_index(src);
    if (src_index == UINT32_MAX) {
        return;
    }

    FOR_URANGE(i, 0, typegraph.len) {
        type* t = typegraph.at[i];
        switch (t->tag) {
        case T_STRUCT:
        case T_UNION:
        case T_UNTYPED_AGGREGATE:
            FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
                if (get_field(t, i)->subtype == src) {
                    get_field(t, i)->subtype = dest;
                    // t->dirty = true;
                }
            }
            break;
        case T_FUNCTION:
            FOR_URANGE(i, 0, t->as_function.params.len) {
                if (t->as_function.params.at[i].subtype == src) {
                    t->as_function.params.at[i].subtype = dest;
                    // t->dirty = true;
                }
            }
            FOR_URANGE(i, 0, t->as_function.returns.len) {
                if (t->as_function.returns.at[i].subtype == src) {
                    t->as_function.returns.at[i].subtype = dest;
                    // t->dirty = true;
                }
            }
            break;
        case T_POINTER:
        case T_SLICE:
        case T_ALIAS:
        case T_DISTINCT:
            if (get_target(t) == src) {
                set_target(t, dest);
                // t->dirty = true;
            }
            break;
        case T_ARRAY:
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

void type_locally_number(type* t, u64* number, int num_set) {
    if (t->type_nums[num_set] != 0) return;

    t->type_nums[num_set] = (*number)++;

    switch (t->tag) {
    case T_STRUCT:
    case T_UNION:
    case T_UNTYPED_AGGREGATE:
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            type_locally_number(get_field(t, i)->subtype, number, num_set);
        }
        break;
    case T_FUNCTION:
        FOR_URANGE(i, 0, t->as_function.params.len) {
            type_locally_number(t->as_function.params.at[i].subtype, number, num_set);
        }
        FOR_URANGE(i, 0, t->as_function.returns.len) {
            type_locally_number(t->as_function.returns.at[i].subtype, number, num_set);
        }
        break;
    case T_ARRAY:
        type_locally_number(t->as_array.subtype, number, num_set);
    case T_POINTER:
    case T_SLICE:
    case T_DISTINCT:
    case T_ALIAS:
        type_locally_number(get_target(t), number, num_set);
        break;
    default:
        break;
    }
}

// do checking on the fly, improved method for ""incomplete"" type graphs
bool otf_types_are_equivalent(type* a, type* b) {
    while (a->tag == T_ALIAS) a = get_target(a);
    while (b->tag == T_ALIAS) b = get_target(b);

    // simple checks
    if (a == b) return true;
    if (a->tag != b->tag) return false;
    if (a->tag < T_meta_INTEGRAL) return true;
    
    // a little more complex
    switch (a->tag) {
    case T_POINTER:
    case T_SLICE:
        if (a->as_reference.mutable != b->as_reference.mutable) return false;
        if (get_target(a) == get_target(b)) return true;
        break;
    case T_STRUCT:
    case T_UNION:
    case T_UNTYPED_AGGREGATE:
        if (a->as_aggregate.fields.len != b->as_aggregate.fields.len) return false;
        bool subtype_equals = true;
        FOR_URANGE(i, 0, a->as_aggregate.fields.len) {
            if (get_field(a, i)->subtype != get_field(b, i)->subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case T_FUNCTION:
        if (a->as_function.params.len != b->as_function.params.len) {
            return false;
        }
        if (a->as_function.returns.len != b->as_function.returns.len) {
            return false;
        }
        subtype_equals = true;
        FOR_URANGE(i, 0, a->as_function.params.len) {
            if (a->as_function.params.at[i].subtype != b->as_function.params.at[i].subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        FOR_URANGE(i, 0, a->as_function.returns.len) {
            if (a->as_function.returns.at[i].subtype != b->as_function.returns.at[i].subtype) {
                subtype_equals = false;
                break;
            }
        }
        if (subtype_equals) return true;
        break;
    case T_ARRAY:
        if (a->as_array.len != b->as_array.len) return false;
        if (a->as_array.subtype == b->as_array.subtype) return true;
        break;
    case T_DISTINCT: // distinct types are VERY strict
        return a == b;
    case T_ADDR:
        return a == b; // this should really not do anything
    default: break;
    }
    
    type_reset_numbers(0);
    type_reset_numbers(1);

    // we have to do some modified parallel TSA
    TODO("too tired to impl this rn");


}

void type_reset_numbers(int num_set) {
    FOR_URANGE(i, 0, typegraph.len) {
        typegraph.at[i]->type_nums[num_set] = 0;
    }
}

type* get_type_from_num(u16 num, int num_set) {
    // printf("FUCK %hu %d\n", num, num_set);
    FOR_URANGE(i, 0, typegraph.len) {
        if (typegraph.at[i]->type_nums[num_set] == num) return typegraph.at[i];
    }
    return NULL;
}

type* make_type(u8 tag) {
    if (typegraph.at == NULL) {
        make_type_graph();
    }
    if (tag < T_meta_INTEGRAL && (typegraph.len >= T_meta_INTEGRAL)) {
        return typegraph.at[tag];
    }

    type* t = malloc(sizeof(type));
    *t = (type){0};
    t->moved = false;
    t->tag = tag;

    switch (tag) {
    case T_STRUCT:
    case T_UNION:
    case T_UNTYPED_AGGREGATE:
        da_init(&t->as_aggregate.fields, 1);
        break;
    case T_ENUM:
        da_init(&t->as_enum.variants, 1);
        t->as_enum.backing_type = typegraph.at[T_I64];
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
    typegraph = (type_graph){0};
    da_init(&typegraph, 3);

    FOR_RANGE(i, 0, T_meta_INTEGRAL) {
        make_type(i);
    }
}

forceinline void add_field(type* s, string name, type* sub) {
    da_append(&s->as_aggregate.fields, ((struct_field){name, sub}));
}

forceinline struct_field* get_field(type* s, size_t i) {
    return &s->as_aggregate.fields.at[i];
}

forceinline void add_variant(type* e, string name, i64 val) {
    da_append(&e->as_enum.variants, ((enum_variant){name, val}));
}

forceinline enum_variant* get_variant(type* e, size_t i) {
    return &e->as_enum.variants.at[i];
}

forceinline bool type_enum_variant_less(enum_variant* a, enum_variant* b) {
    if (a->enum_val == b->enum_val) {
        // use string names
        return string_cmp(a->name, b->name) < 0;
    } else {
        return a->enum_val < b->enum_val;
    }
}

forceinline void set_target(type* p, type* dest) {
    p->as_reference.subtype = dest;
}

forceinline type* get_target(type* p) {
    return p->as_reference.subtype;
}

u64 get_index(type* t) {
    FOR_URANGE(i, 0, typegraph.len) {
        if (typegraph.at[i] == t) return i;
    }
    return UINT32_MAX;
}

void print_type_graph() {
    printf("-------------------------\n");
    FOR_URANGE(i, 0, typegraph.len) {
        type* t = typegraph.at[i];
        if (t->moved) continue;
        // printf("%-2zu   [%-2hu, %-2hu]\t", i, t->type_nums[0], t->type_nums[1]);
        printf("%-2zu\t", i);
        // printf(t->dirty ? "[dirty]\t" : "[clean]\t");
        switch (t->tag){
        case T_NONE:    printf("(none)\n"); break;
        case T_I8:      printf("i8\n");     break;
        case T_I16:     printf("i16\n");    break;
        case T_I32:     printf("i32\n");    break;
        case T_I64:     printf("i64\n");    break;
        case T_U8:      printf("u8\n");     break;
        case T_U16:     printf("u16\n");    break;
        case T_U32:     printf("u32\n");    break;
        case T_U64:     printf("u64\n");    break;
        case T_F16:     printf("f16\n");    break;
        case T_F32:     printf("f32\n");    break;
        case T_F64:     printf("f64\n");    break;
        case T_ADDR:    printf("addr\n");   break;
        case T_ALIAS:
            printf("alias %zu\n", get_index(get_target(t)));
            break;
        case T_DISTINCT:
            printf("distinct %zu\n", get_index(get_target(t)));
            break;
        case T_POINTER:
            printf("pointer %zu\n", get_index(get_target(t)));
            break;
        case T_SLICE:
            printf("slice %zu\n", get_index(get_target(t)));
            break;
        case T_ARRAY:
            printf("array %zu\n", get_index(t->as_array.subtype));
            break;
        case T_STRUCT:
        case T_UNION:
            printf(t->tag == T_STRUCT ? "struct\n" : "union\n");
            FOR_URANGE(field, 0, t->as_aggregate.fields.len) {
                printf("\t\t.%s : %zu\n", get_field(t, field)->name, get_index(get_field(t, field)->subtype));
            }
            break;
        
        default:
            printf("unknown tag %d\n", t->tag);
            break;
        }
    }
}

// is type unboundedly recursive (have infinite size)?
bool type_is_infinite(type* t) {
    if (t->visited) return true;

    t->visited = true;
    bool is_inf = false;

    switch (t->tag) {
    case T_STRUCT:
    case T_UNION:
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            if (type_is_infinite(get_field(t, i)->subtype)) {
                is_inf = true;
                break;
            }
        }
        break;
    case T_FUNCTION:
        FOR_URANGE(i, 0, t->as_function.params.len) {
            if (type_is_infinite(t->as_function.params.at[i].subtype)) {
                is_inf = true;
                break;
            }
        }
        FOR_URANGE(i, 0, t->as_function.returns.len) {
            if (type_is_infinite(t->as_function.returns.at[i].subtype)) {
                is_inf = true;
                break;
            }
        }
        break;
    case T_ARRAY:
        is_inf = type_is_infinite(t->as_array.subtype);
        break;
    case T_POINTER:
    case T_SLICE:
        break;

    case T_DISTINCT:
    case T_ALIAS:
        type_is_infinite(get_target(t));
        break;
    default:
        break;
    }

    t->visited = false;
    return is_inf;
}

u32 static size_of_internal(type* t) {
    if (t->size != UINT32_MAX) return t->size;

    switch (t->tag) {
    case T_NONE:
        return t->size = 0;
    case T_I8:
    case T_U8:
    case T_BOOL:
        return t->size = 1;
    case T_I16:
    case T_U16:
    case T_F16:
        return t->size = 2;
    case T_I32:
    case T_U32:
    case T_F32:
        return t->size = 4;
    case T_I64:
    case T_U64:
    case T_F64:
    case T_POINTER:
    case T_ADDR:
    case T_FUNCTION: // remember, its a function POINTER!
        return t->size = 8;
    case T_SLICE:
        return t->size = 16;
    case T_ENUM:
        return t->size = size_of_internal(t->as_enum.backing_type);
    case T_ALIAS:
    case T_DISTINCT:
        return t->size = size_of_internal(t->as_reference.subtype);
    case T_ARRAY:
        return t->size = size_of_internal(t->as_array.subtype) * t->as_array.len;
    case T_UNION: {
        u64 max_size = 0;
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            u64 size = size_of_internal(get_field(t, i)->subtype);
            if (size > max_size) max_size = size;
        }
        return max_size;
        } break;
    case T_STRUCT: {
        u64 full_size = 0;
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            full_size += size_of_internal(get_field(t, i)->subtype);
        }
        return full_size;
        } break;
    default:
        CRASH("unreachable");
    }
}

u32 type_real_size_of(type* t) {
    if (type_is_infinite(t)) return UINT32_MAX;
    return size_of_internal(t);
}

u32 static align_of_internal(type* t) {
    if (t->align != UINT32_MAX) return t->align;

    switch (t->tag) {
    case T_NONE:
        return t->align = 0;
    case T_I8:
    case T_U8:
        return t->align = 1;
    case T_I16:
    case T_U16:
    case T_F16:
        return t->align = 2;
    case T_I32:
    case T_U32:
    case T_F32:
        return t->align = 4;
    case T_I64:
    case T_U64:
    case T_F64:
    case T_POINTER:
    case T_ADDR:
    case T_FUNCTION: // remember, its a function POINTER!
    case T_SLICE:
        return t->align = 8;
    case T_ENUM:
        return t->align = align_of_internal(t->as_enum.backing_type);
    case T_ALIAS:
    case T_DISTINCT:
        return t->align = align_of_internal(t->as_reference.subtype);
    case T_ARRAY:
        return t->align = align_of_internal(t->as_array.subtype);
    
    case T_UNION:
    case T_STRUCT: {
        u64 max_align = 0;
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            u64 align = align_of_internal(get_field(t, i)->subtype);
            if (align > max_align) max_align = align;
        }
        return t->align = max_align;
        } break;
    default:
        CRASH("unreachable");
    }
}

u32 type_real_align_of(type* t) {
    if (type_is_infinite(t)) return UINT32_MAX;
    return align_of_internal(t);
}