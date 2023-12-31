#include "orbit.h"

// string functions for orbit.h

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