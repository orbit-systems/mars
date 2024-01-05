#pragma once
#define ORBITSTR_H

// strings and string-related utils.

#include "orbit.h"

typedef struct string_s {
    char* raw;
    u32   len;
} string;

#define NULL_STR ((string){NULL, 0})
#define is_null_str(str) ((str).raw == NULL)

#define string_make(ptr, len) ((string){(ptr), (len)})
#define string_len(s) ((s).len)
#define string_raw(s) ((s).raw)
#define is_within(haystack, needle) (((haystack).raw <= (needle).raw) && ((haystack).raw + (haystack).len >= (needle).raw + (needle).len))
#define substring(str, start, end_excl) ((string){(str).raw + (start), (end_excl) - (start)})
#define substring_len(str, start, len) ((string){(str).raw + (start), (len)})
#define can_be_cstring(str) ((str).raw[(str).len] == '\0')

string to_string(char* cstring);
char*  to_cstring(string str); // this allocates
void   printstr(string str);

string  string_alloc(size_t len);
#define string_free(str) free(str.raw)
string  string_clone(string str); // this allocates as well
string  string_concat(string a, string b); // allocates

int  string_cmp(string a, string b);
bool string_eq(string a, string b);
bool string_ends_with(string source, string ending);

// use #define ORBITSTR_IMPLEMENTATION before including the header to generate the actual code

#ifdef ORBITSTR_IMPLEMENTATION

string string_concat(string a, string b) {
    string c = string_alloc(a.len + b.len);
    FOR_RANGE_EXCL(i, 0, a.len)
        c.raw[i] = a.raw[i];
    FOR_RANGE_EXCL(i, 0, b.len)
        c.raw[a.len] = b.raw[i];
    return c;
}

bool string_ends_with(string source, string ending) {
    if (source.len < ending.len) return false;

    return string_eq(substring_len(source, source.len-ending.len, ending.len), ending);
}

string string_alloc(size_t len) {
    char* raw = malloc(len);
    if (raw == NULL) return NULL_STR;
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
    FOR_RANGE_EXCL(i, 0, a.len) {
        if (a.raw[i] != b.raw[i]) return false;
    }
    return true;
}

string to_string(char* cstring) {
    return (string){cstring, strlen(cstring)};
}

char* to_cstring(string str) {
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
    while (text[c] != '\0' && c < len)
        putchar(text[c++]);
}

void printstr(string str) {
    printn(str.raw, str.len);
}
#endif