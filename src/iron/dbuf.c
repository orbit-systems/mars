#include <stdio.h>
#include <stdarg.h>

#include "iron/iron.h"

// data buffer functions

void fe_db_init(FeDataBuffer* buf, usize cap) {
    if (cap < 2) cap = 2;
    buf->at = fe_malloc(cap);
    buf->cap = cap;
    buf->len = 0;
}

void fe_db_clone(FeDataBuffer* dst, FeDataBuffer* src) {
    fe_db_init(dst, src->cap);
    dst->len = src->len;
    memcpy(dst->at, src->at, src->len);
}

char* fe_db_clone_to_cstring(FeDataBuffer* buf) {
    char* s = fe_malloc(buf->len + 1);
    memcpy(s, buf->at, buf->len);
    s[buf->len] = '\0';
    return s;
}

void fe_db_reserve(FeDataBuffer* buf, usize more) {
    if (buf->cap >= buf->len + more) return;

    while (buf->cap < buf->len + more) {
        buf->cap += buf->cap >> 1;
    }

    buf->at = fe_realloc(buf->at, buf->cap);
}

// reserve until buf->cap == new_cap
// does nothing when buf->cap >= new_cap
void fe_db_reserve_until(FeDataBuffer* buf, usize new_cap) {
    if (buf->cap >= new_cap) return;
    buf->at = fe_realloc(buf->at, new_cap);
    buf->cap = new_cap;
}

usize fe_db_write(FeDataBuffer* buf, const void* ptr, usize len) {
    fe_db_reserve(buf, len);
    memcpy(buf->at + buf->len, ptr, len);
    buf->len += len;
    return len;
}

usize fe_db_writecstr(FeDataBuffer* buf, const char* s) {
    usize slen = strlen(s);
    fe_db_reserve(buf, slen);
    memcpy(buf->at + buf->len, s, slen);
    buf->len += slen;
    return slen;
}
usize fe_db_write8(FeDataBuffer* buf, u8 data) {
    fe_db_reserve(buf, 1);
    buf->at[buf->len++] = data;
    return 1;
}

usize fe_db_write16(FeDataBuffer* buf, u16 data) {
    fe_db_reserve(buf, sizeof(data));
    memcpy(buf->at + buf->len, &data, sizeof(data));
    buf->len += sizeof(data);
    return sizeof(data);
}

usize fe_db_write32(FeDataBuffer* buf, u32 data) {
    fe_db_reserve(buf, sizeof(data));
    memcpy(buf->at + buf->len, &data, sizeof(data));
    buf->len += sizeof(data);
    return sizeof(data);
}

usize fe_db_write64(FeDataBuffer* buf, u64 data) {
    fe_db_reserve(buf, sizeof(data));
    memcpy(buf->at + buf->len, &data, sizeof(data));
    buf->len += sizeof(data);
    return sizeof(data);
}

usize fe_db_writef(FeDataBuffer* buf, const char* fmt, ...) {
    va_list varargs;
    va_start(varargs, fmt);

    thread_local static char chars[1000];
    memset(chars, 0, sizeof(chars));

    usize len = vsnprintf(chars, sizeof(chars), fmt, varargs);
    fe_db_writecstr(buf, chars);
    return len;
}

void fe_db_destroy(FeDataBuffer* buf) {
    fe_free(buf->at);
    *buf = (FeDataBuffer){};
}
