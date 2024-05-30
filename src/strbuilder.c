#include "strbuilder.h"

void sb_init(StringBuilder* sb) {
    da_init(sb, SB_INITIAL_CAPACITY);
    sb->alloca = arena_make(SB_ARENA_BLOCK_SIZE);
}

void sb_destroy(StringBuilder* sb) {
    da_destroy(sb);
    arena_delete(&sb->alloca);
}

void sb_append(StringBuilder* sb, string s) {
    // copy string to allocator
    char* p = arena_alloc(&sb->alloca, s.len, 1);
    memcpy(p, s.raw, s.len);

    da_append(sb, ((string){.raw = p, .len = s.len}));
}

void sb_append_c(StringBuilder* sb, char* s) {
    // copy string to arena
    size_t slen = strlen(s);
    char* p = arena_alloc(&sb->alloca, slen, 1);
    memcpy(p, s, slen);

    da_append(sb, ((string){ .raw = p, .len = slen }));
}

size_t sb_len(StringBuilder* sb) {
    size_t len = 0;
    foreach(string s, *sb) {
        len += s.len;
    }
    return len;
}

void sb_write(StringBuilder* sb, char* buffer) {
    foreach(string s, *sb) {
        memcpy(buffer, s.raw, s.len);
        buffer += s.len;
    }
}