#include "iron.h"

// data buffer functions

FeDataBuffer fe_db_new(size_t initial_capacity) {
    FeDataBuffer buf = {0};
    buf.at = fe_malloc(initial_capacity);
    buf.cap = initial_capacity;
    buf.len = 0;
    return buf;
}

FeDataBuffer fe_db_clone(FeDataBuffer* buf) {
    FeDataBuffer new_buf = fe_db_new(buf->cap);
    new_buf.len = buf->len;
    memcpy(new_buf.at, buf->at, buf->len);
    return new_buf;
}
FeDataBuffer fe_db_new_from_file(FeDataBuffer* buf, string path) {
    TODO("");
}

string fe_db_clone_to_string(FeDataBuffer* buf) {
    string s;
    s.raw = fe_malloc(buf->len);
    s.len = buf->len;
    memcpy(s.raw, buf->at, s.len);
    return s;
}
char* fe_db_clone_to_cstring(FeDataBuffer* buf) {
    char* s = fe_malloc(buf->len + 1);
    memcpy(s, buf->at, buf->len);
    s[buf->len] = '\0';
    return s;
}

// make sure buf->cap >= buf->len + more
void fe_db_reserve(FeDataBuffer* buf, size_t more) {
    if (buf->cap >= buf->len + more) return;
    
    buf->at = fe_realloc(buf->at, buf->len + more);
    buf->cap = buf->len + more;
}

// reserve until buf->cap == new_cap
// does nothing when buf->cap >= new_cap
void fe_db_reserve_until(FeDataBuffer* buf, size_t new_cap) {
    if (buf->cap >= new_cap) return;
    buf->at = fe_realloc(buf->at, new_cap);
    buf->cap = new_cap;
}

// append data to end of buffer
size_t fe_db_write_string(FeDataBuffer* buf, string s) {
    return fe_db_write_bytes(buf, s.raw, s.len);
}

size_t fe_db_write_bytes(FeDataBuffer* buf, void* ptr, size_t len) {
    fe_db_reserve(buf, len);
    memcpy(buf->at + buf->len, ptr, len);
    buf->len += len;
    return len;
}

size_t fe_db_write_cstring(FeDataBuffer* buf, char* s, bool NUL) {
    size_t slen = strlen(s);
    fe_db_reserve(buf, slen + (u64)NUL);
    memcpy(buf->at + buf->len, s, slen);
    buf->len += slen + (u64)NUL;
    if (NUL) buf->at[buf->len - 1] = '\0';
    return slen;
}
size_t fe_db_write_8(FeDataBuffer* buf, u8 data) {
    fe_db_reserve(buf, 1);
    buf->at[buf->len++] = data;
    return 1;
}

size_t fe_db_write_16(FeDataBuffer* buf, u16 data) {
    fe_db_reserve(buf, sizeof(data));
    memcpy(buf->at + buf->len, &data, sizeof(data));
    buf->len =+ sizeof(data);
    return sizeof(data);
}

size_t fe_db_write_32(FeDataBuffer* buf, u32 data) {
    fe_db_reserve(buf, sizeof(data));
    memcpy(buf->at + buf->len, &data, sizeof(data));
    buf->len =+ sizeof(data);
    return sizeof(data);
}

size_t fe_db_write_64(FeDataBuffer* buf, u64 data) {
    fe_db_reserve(buf, sizeof(data));
    memcpy(buf->at + buf->len, &data, sizeof(data));
    buf->len =+ sizeof(data);
    return sizeof(data);
}

size_t fe_db_write_format(FeDataBuffer* buf, char* fmt, ...) {
    va_list varargs;
    va_start(varargs, fmt);

    static char chars[1000];
    memset(chars, 0, sizeof(chars));

    vsprintf(chars, fmt, varargs);
    fe_db_write_cstring(buf, chars, false);
    return strlen(chars);
}

// insert data at a certain point, growing the buffer
size_t fe_db_insert_string(FeDataBuffer* buf, size_t at, string s);
size_t fe_db_insert_bytes(FeDataBuffer* buf, size_t at, void* ptr, size_t len);
size_t fe_db_insert_cstring(FeDataBuffer* buf, size_t at, char* s);
size_t fe_db_insert_8(FeDataBuffer* buf, size_t at, u8 data);
size_t fe_db_insert_16(FeDataBuffer* buf, size_t at, u16 data);
size_t fe_db_insert_32(FeDataBuffer* buf, size_t at, u32 data);
size_t fe_db_insert_64(FeDataBuffer* buf, size_t at, u64 data);

// overwrite data at a certain point, growing the buffer if needed
size_t fe_db_overwrite_string(FeDataBuffer* buf, size_t at, string s);
size_t fe_db_overwrite_bytes(FeDataBuffer* buf, size_t at, void* ptr, size_t len);
size_t fe_db_overwrite_cstring(FeDataBuffer* buf, size_t at, char* s);
size_t fe_db_overwrite_8(FeDataBuffer* buf, size_t at, u8 data);
size_t fe_db_overwrite_16(FeDataBuffer* buf, size_t at, u16 data);
size_t fe_db_overwrite_32(FeDataBuffer* buf, size_t at, u32 data);
size_t fe_db_overwrite_64(FeDataBuffer* buf, size_t at, u64 data);

// TODO add read functions