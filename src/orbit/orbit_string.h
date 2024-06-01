#pragma once
#define ORBIT_STRING_H

// strings and string-related utils.

// #define CSTRING_COMPATIBILITY_MODE
// allocates an extra character for null termination outside of the string bounds.
// probably recommended if you interface a lot with standard C APIs and dont want clone_to_cstring allocations everywhere.

typedef struct string_s {
    char* raw;
    u32   len;
} string;

#define NULL_STR ((string){NULL, 0})
#define is_null_str(str) ((str).raw == NULL)

#define str_fmt "%.*s"
#define str_arg(str) (int)(str).len, (str).raw

#define string_make(ptr, len) ((string){(ptr), (len)})
#define string_len(s) ((s).len)
#define string_raw(s) ((s).raw)
#define is_within(haystack, needle) (((haystack).raw <= (needle).raw) && ((haystack).raw + (haystack).len >= (needle).raw + (needle).len))
#define substring(str, start, end) ((string){(str).raw + (start), (end) - (start)})
#define substring_len(str, start, len) ((string){(str).raw + (start), (len)})
#define str(cstring) ((string){(cstring), strlen((cstring))})
#define constr(cstring) ((string){(char*)cstring, sizeof(cstring)-1})

char*  clone_to_cstring(string str); // this allocates
void   printstr(string str);
string strprintf(char* format, ...);

string  string_alloc(size_t len);
#define string_free(str) free(str.raw)
string  string_clone(string str); // this allocates as well
string  string_concat(string a, string b); // allocates
void  string_concat_buf(string buf, string a, string b); // this does not

int  string_cmp(string a, string b);
bool string_eq(string a, string b);
bool string_ends_with(string source, string ending);

#ifdef ORBIT_IMPLEMENTATION
string strprintf(char* format, ...) {
    string c = NULL_STR;
    va_list a;
    va_start(a, format);
    va_list b;
    va_copy(b, a);
    size_t bufferlen = 1 + vsnprintf("", 0, format, a);
    c = string_alloc(bufferlen);
    vsnprintf(c.raw, c.len, format, b);
    c.len--;
    va_end(a);
    va_end(b);
    return c;
}

string string_concat(string a, string b) {
    string c = string_alloc(a.len + b.len);
    for_range(i, 0, a.len) c.raw[i] = a.raw[i];
    for_range(i, 0, b.len) c.raw[a.len + i] = b.raw[i];
    return c;
}

void string_concat_buf(string buf, string a, string b) {
    for_range(i, 0, a.len) buf.raw[i] = a.raw[i];
    for_range(i, 0, b.len) buf.raw[a.len + i] = b.raw[i];
}

bool string_ends_with(string source, string ending) {
    if (source.len < ending.len) return false;

    return string_eq(substring_len(source, source.len-ending.len, ending.len), ending);
}

string string_alloc(size_t len) {
    #ifdef CSTRING_COMPATIBILITY_MODE
    char* raw = malloc(len + 1);
    #else
    char* raw = malloc(len);
    #endif

    if (raw == NULL) return NULL_STR;

    memset(raw, '\0', len);

    #ifdef CSTRING_COMPATIBILITY_MODE
    raw[len] = '\0';
    #endif

    return (string){raw, len};

}

int string_cmp(string a, string b) {
    // copied from odin's implementation lmfao
    int res = memcmp(a.raw, b.raw, min(a.len, b.len));
    if (res == 0 && a.len != b.len) return a.len <= b.len ? -1 : 1;
    else if (a.len == 0 && b.len == 0) return 0;
    return res;
}

bool string_eq(string a, string b) {
    if (a.len != b.len) return false;
    for_range(i, 0, a.len) {
        if (a.raw[i] != b.raw[i]) return false;
    }
    return true;
}

char* clone_to_cstring(string str) {
    if (is_null_str(str)) return "";

    char* cstr = malloc(str.len + 1);
    if (cstr == NULL) return NULL;
    memcpy(cstr, str.raw, str.len);
    cstr[str.len] = '\0';
    return cstr;
}

string string_clone(string str) {
    string new_str = string_alloc(str.len);
    if (memmove(new_str.raw, str.raw, str.len) != new_str.raw) return NULL_STR;
    return new_str;
}

void printn(char* text, size_t len) {
    size_t c = 0;
    while (c < len && text[c] != '\0')
        putchar(text[c++]);
}

void printstr(string str) {
    printn(str.raw, str.len);
}
#endif