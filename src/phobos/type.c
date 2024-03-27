#include "type.h"

#include <locale.h>

// type engine

type_graph tg;

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
    FOR_URANGE(i, 0, tg.len) {
        type* t = tg.at[i];
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
        // num_of_types = tg.len;
        keep_going = false;
        FOR_URANGE(i, T_meta_INTEGRAL, tg.len) {
            bool executed_TSA_at_all = false;
            if (tg.at[i]->tag == T_ALIAS) continue;
            if (tg.at[i]->tag == T_DISTINCT) continue;
            if (tg.at[i]->moved) continue;
            FOR_URANGE(j, i+1, tg.len) {
                if (tg.at[j]->tag == T_ALIAS) continue;
                if (tg.at[j]->tag == T_DISTINCT) continue;
                if (tg.at[j]->moved) continue;
                if (!(tg.at[i]->dirty || tg.at[j]->dirty)) continue;
                bool executed_TSA = false;
                if (types_are_equivalent(tg.at[i], tg.at[j], &executed_TSA)) {
                    da_append(&equalities, ((type_pair){tg.at[i], tg.at[j]}));
                }
                if (executed_TSA) {
                    executed_TSA_at_all = true;
                    type_reset_numbers(1);
                }
            }
            if (executed_TSA_at_all) {
                type_reset_numbers(0);
            }
            LOG("compared all to %p (%4zu / %4zu)\n", tg.at[i], i+1, tg.len);
            tg.at[i]->dirty = false;
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
            dest->dirty = true;
            keep_going = true;
            LOG("merged %p <- %p (%4zu / %4zu)\n", dest, src, equalities.len-i, equalities.len);
            
        }
        // LOG("equalities merged\n");
        da_clear(&equalities);


        FOR_URANGE(i, 0, tg.len) {
            if (tg.at[i]->moved) {
                da_unordered_remove_at(&tg, i);
                i--;
            }
        }
    }

    da_destroy(&equalities);
}

bool types_are_equivalent(type* restrict a, type* restrict b, bool* executed_TSA) {

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
        if (a->as_reference.constant != b->as_reference.constant) return false;
        if (get_target(a) == get_target(b)) return true;
        break;
    case T_STRUCT:
    case T_UNION:
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

bool type_element_equivalent(type* restrict a, type* restrict b, int num_set_a, int num_set_b) {
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
            if (strcmp(get_variant(a, i)->name, get_variant(b, i)->name) != 0) {
                return false;
            }
        }
        break;
    case T_STRUCT:
    case T_UNION:
        if (a->as_aggregate.fields.len != b->as_aggregate.fields.len) {
            return false;
        }
        FOR_URANGE(i, 0, a->as_aggregate.fields.len) {
            if (strcmp(get_field(a, i)->name, get_field(b, i)->name) != 0) {
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
            if (strcmp(a->as_function.params.at[i].name, b->as_function.params.at[i].name) != 0) {
                return false;
            }
            if (a->as_function.params.at[i].subtype->type_nums[num_set_a] != b->as_function.params.at[i].subtype->type_nums[num_set_b]) {
                return false;
            }
        }
        FOR_URANGE(i, 0, a->as_function.returns.len) {
            if (strcmp(a->as_function.returns.at[i].name, b->as_function.returns.at[i].name) != 0) {
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
        if (a->as_reference.constant != b->as_reference.constant) return false;
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

void merge_type_references(type* restrict dest, type* restrict src, bool disable) {

    u64 src_index = get_index(src);
    if (src_index == UINT64_MAX) {
        return;
    }

    FOR_URANGE(i, 0, tg.len) {
        type* t = tg.at[i];
        switch (t->tag) {
        case T_STRUCT:
        case T_UNION:
            FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
                if (get_field(t, i)->subtype == src) {
                    get_field(t, i)->subtype = dest;
                    t->dirty = true;
                }
            }
            break;
        case T_FUNCTION:
            FOR_URANGE(i, 0, t->as_function.params.len) {
                if (t->as_function.params.at[i].subtype == src) {
                    t->as_function.params.at[i].subtype = dest;
                    t->dirty = true;
                }
            }
            FOR_URANGE(i, 0, t->as_function.returns.len) {
                if (t->as_function.returns.at[i].subtype == src) {
                    t->as_function.returns.at[i].subtype = dest;
                    t->dirty = true;
                }
            }
            break;
        case T_POINTER:
        case T_SLICE:
        case T_ALIAS:
        case T_DISTINCT:
            if (get_target(t) == src) {
                set_target(t, dest);
                t->dirty = true;
            }
            break;
        case T_ARRAY:
            if (t->as_array.subtype == src) {
                t->as_array.subtype = dest;
                t->dirty = true;
            }
            break;
        default:
            break;
        }
    }

    // printf("--- %zu\n", src_index);
    if (disable) src->moved = dest;
    dest->dirty = true;
}

void type_locally_number(type* restrict t, u64* number, int num_set) {
    if (t->type_nums[num_set] != 0) return;

    t->type_nums[num_set] = (*number)++;

    switch (t->tag) {
    case T_STRUCT:
    case T_UNION:
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

void type_reset_numbers(int num_set) {
    FOR_URANGE(i, 0, tg.len) {
        tg.at[i]->type_nums[num_set] = 0;
    }
}

type* get_type_from_num(u16 num, int num_set) {
    // printf("FUCK %hu %d\n", num, num_set);
    FOR_URANGE(i, 0, tg.len) {
        if (tg.at[i]->type_nums[num_set] == num) return tg.at[i];
    }
    return NULL;
}

type* make_type(u8 tag) {
    if (tag < T_meta_INTEGRAL && (tg.len >= T_meta_INTEGRAL)) {
        return tg.at[tag];
    }

    type* t = malloc(sizeof(type));
    *t = (type){0};
    t->moved = false;
    t->tag = tag;

    switch (tag) {
    case T_STRUCT:
    case T_UNION:
        da_init(&t->as_aggregate.fields, 1);
        break;
    case T_ENUM:
        da_init(&t->as_enum.variants, 1);
        t->as_enum.backing_type = tg.at[T_I64];
        break;
    default: break;
    }
    t->dirty = true;
    da_append(&tg, t);
    return t;
}

void make_type_graph() {
    tg = (type_graph){0};
    da_init(&tg, 3);

    make_type(T_NONE);
    make_type(T_I8);
    make_type(T_I16);
    make_type(T_I32);
    make_type(T_I64);
    make_type(T_U8);
    make_type(T_U16);
    make_type(T_U32);
    make_type(T_U64);
    make_type(T_F16);
    make_type(T_F32);
    make_type(T_F64);
    make_type(T_ADDR);
}

void add_field(type* restrict s, char* name, type* restrict sub) {
    if (s->tag != T_STRUCT && s->tag != T_UNION) return;
    da_append(&s->as_aggregate.fields, ((struct_field){name, sub}));
}

struct_field* get_field(type* restrict s, size_t i) {
    if (s->tag != T_STRUCT && s->tag != T_UNION) return NULL;
    return &s->as_aggregate.fields.at[i];
}

void add_variant(type* restrict e, char* name, i64 val) {
    if (e->tag != T_ENUM) return;
    da_append(&e->as_enum.variants, ((enum_variant){name, val}));
}

enum_variant* get_variant(type* restrict e, size_t i) {
    if (e->tag != T_ENUM) return NULL;
    return &e->as_enum.variants.at[i];
}

bool type_enum_variant_less(enum_variant* a, enum_variant* b) {
    if (a->enum_val == b->enum_val) {
        // use string names
        return strcmp(a->name, b->name) < 0;
    } else {
        return a->enum_val < b->enum_val;
    }
}

void set_target(type* restrict p, type* restrict dest) {
    if (p->tag != T_POINTER && 
        p->tag != T_SLICE && 
        p->tag != T_ALIAS &&
        p->tag != T_DISTINCT) return;
    p->as_reference.subtype = dest;
}

type* restrict get_target(type* restrict p) {
    if (p->tag != T_POINTER && 
        p->tag != T_SLICE && 
        p->tag != T_ALIAS &&
        p->tag != T_DISTINCT) return NULL;
    return p->as_reference.subtype;
}

u64 get_index(type* restrict t) {
    FOR_URANGE(i, 0, tg.len) {
        if (tg.at[i] == t) return i;
    }
    return UINT64_MAX;
}

void print_type_graph() {
    printf("-------------------------\n");
    FOR_URANGE(i, 0, tg.len) {
        type* t = tg.at[i];
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
bool type_is_infinite(type* restrict t) {
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

u64 static size_of_internal(type* restrict t) {
    switch (t->tag) {
    case T_NONE:
        return 0;
    case T_I8:
    case T_U8:
        return 1;
    case T_I16:
    case T_U16:
    case T_F16:
        return 2;
    case T_I32:
    case T_U32:
    case T_F32:
        return 4;
    case T_I64:
    case T_U64:
    case T_F64:
    case T_POINTER:
    case T_ADDR:
    case T_FUNCTION: // remember, its a function POINTER!
        return 8;
    case T_SLICE:
        return 16;
    case T_ENUM:
        return size_of_internal(t->as_enum.backing_type);
    case T_ALIAS:
    case T_DISTINCT:
        return size_of_internal(t->as_reference.subtype);
    case T_ARRAY:
        return size_of_internal(t->as_array.subtype) * t->as_array.len;
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

u64 type_real_size_of(type* restrict t) {
    if (type_is_infinite(t)) return UINT64_MAX;
    return size_of_internal(t);
}

u64 static align_of_internal(type* restrict t) {
    switch (t->tag) {
    case T_NONE:
        return 0;
    case T_I8:
    case T_U8:
        return 1;
    case T_I16:
    case T_U16:
    case T_F16:
        return 2;
    case T_I32:
    case T_U32:
    case T_F32:
        return 4;
    case T_I64:
    case T_U64:
    case T_F64:
    case T_POINTER:
    case T_ADDR:
    case T_FUNCTION: // remember, its a function POINTER!
    case T_SLICE:
        return 8;
    case T_ENUM:
        return align_of_internal(t->as_enum.backing_type);
    case T_ALIAS:
    case T_DISTINCT:
        return align_of_internal(t->as_reference.subtype);
    case T_ARRAY:
        return align_of_internal(t->as_array.subtype);
    
    case T_UNION:
    case T_STRUCT: {
        u64 max_align = 0;
        FOR_URANGE(i, 0, t->as_aggregate.fields.len) {
            u64 align = align_of_internal(get_field(t, i)->subtype);
            if (align > max_align) max_align = align;
        }
        return max_align;
        } break;
    default:
        CRASH("unreachable");
    }
}

u64 type_real_align_of(type* restrict t) {
    if (type_is_infinite(t)) return UINT64_MAX;
    return align_of_internal(t);
}