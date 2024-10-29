#include "common/strbuilder.h"

void sb_init(StringBuilder* sb) {
    sb->len = 0;
    sb->cap = 256;
    sb->buffer = malloc(sb->cap);
}

void sb_destroy(StringBuilder* sb) {
    free(sb->buffer);
    *sb = (StringBuilder){0};
}

void sb_append(StringBuilder* sb, string s) {
    if (sb->len + s.len > sb->cap) {
        sb->buffer = realloc(sb->buffer, sb->cap * 2);
        sb->cap *= 2;
    }
    memcpy(&sb->buffer[sb->len], s.raw, s.len);
    sb->len += s.len;
}

void sb_append_c(StringBuilder* sb, const char* s) {
    if (sb->len + strlen(s) > sb->cap) {
        sb->buffer = realloc(sb->buffer, sb->cap * 2);
        sb->cap *= 2;
    }
    memcpy(&sb->buffer[sb->len], s, strlen(s));
    sb->len += strlen(s);
}
void sb_printf(StringBuilder* sb, const char* fmt, ...) {
    va_list varargs;
    va_start(varargs, fmt);

    static char buf[1000];
    memset(buf, 0, sizeof(buf));

    vsprintf(buf, fmt, varargs);
    sb_append_c(sb, buf);
}

size_t sb_len(StringBuilder* sb) {
    return sb->len;
}

void sb_write(StringBuilder* sb, char* buffer) {
    memcpy(buffer, sb->buffer, sb->len);
}
