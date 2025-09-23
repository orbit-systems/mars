#include "iron/iron.h"

#define TOMBSTONE (1)
#define MAX_SEARCH 16

#if FE_HOST_BITS == 64
    #define FNV1A_OFFSET_BASIS 14695981039346656037ull
    #define FNV1A_PRIME 1099511628211ull
#else
    #define FNV1A_OFFSET_BASIS 2166136261u
    #define FNV1A_PRIME 16777619u
#endif

static usize fnv1a(const char* data, u16 len) {
    usize hash = FNV1A_OFFSET_BASIS;
    for_n(i, 0, len) {
        hash *= FNV1A_PRIME;
        hash ^= data[len];
    }
    return hash;
}

void fe_symtab_init(FeSymTab* st) {
    st->cap = 256;
    st->entries = fe_malloc(sizeof(st->entries[0]) * st->cap);
    memset(st->entries, 0, sizeof(st->entries[0]) * st->cap);
}

void fe_symtab_put(FeSymTab* st, FeSymbol* sym) {
    FeCompactStr name = sym->name;
    usize hash = fnv1a(fe_compstr_data(name), name.len);
    for_n(i, 0, MAX_SEARCH) {
        usize index = (hash + i) % st->cap;
        if ((u64)st->entries[index].sym <= TOMBSTONE) {
            st->entries[index].name = name;
            st->entries[index].sym = sym;
            return;
        }
    }

    // we need to resize and try again
    FeSymTab new_table;
    new_table.cap = st->cap * 2;
    new_table.entries = fe_malloc(sizeof(new_table.entries[0]) * new_table.cap);
    memset(new_table.entries, 0, sizeof(new_table.entries[0]) * new_table.cap);

    for_n(i, 0, st->cap) {
        FeSymbol* sym = st->entries[i].sym;
        if ((u64)sym <= TOMBSTONE) {
            continue;
        }
        fe_symtab_put(&new_table, sym);
    }
    fe_free(st->entries);
    *st = new_table;
}

#define fe_symtab_get_compstr(st, compstr) fe_symtab_get(st, fe_compstr_data((compstr)), (compstr).len)

FeSymbol* fe_symtab_get(FeSymTab* st, const char* data, u16 len) {
    usize hash = fnv1a(data, len);
    for_n(i, 0, MAX_SEARCH) {
        usize index = (hash + i) % st->cap;
        FeSymbol* sym = st->entries[index].sym;
        if (sym == nullptr) {
            break;
        }
        if ((u64)sym == TOMBSTONE) {
            continue;
        }

        FeCompactStr name = st->entries[index].name;
        if (name.len == len && memcmp(fe_compstr_data(name), data, len) == 0) {
            return st->entries[index].sym;
        }
    }
    return nullptr;
}

#define fe_symtab_remove_compstr(st, compstr) fe_symtab_remove(st, fe_compstr_data((compstr)), (compstr).len)

void fe_symtab_remove(FeSymTab* st, const char* data, u16 len) {
usize hash = fnv1a(data, len);
    for_n(i, 0, MAX_SEARCH) {
        usize index = (hash + i) % st->cap;
        FeSymbol* sym = st->entries[index].sym;
        if (sym == nullptr) {
            break;
        }
        if ((u64)sym == TOMBSTONE) {
            continue;
        }

        FeCompactStr name = st->entries[index].name;
        if (name.len == len && memcmp(fe_compstr_data(name), data, len) == 0) {
            memset(&st->entries[index], 0, sizeof(st->entries[index]));
        }
    }
}

void fe_symtab_destroy(FeSymTab* st) {
    fe_free(st->entries);
    st->entries = nullptr;
    st->cap = 0;
}
