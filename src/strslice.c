#include "orbit.h"
#include "strslice.h"

int string_cmp(string a, string b) {
    // copied from odin's implementation lmfao
    int res = memcmp(a.raw, b.raw, min(a.len, b.len));
    if (res == 0 && a.len != b.len) return a.len <= b.len ? -1 : 1;
	else if (a.len == 0 && b.len == 0) return 0;
	return res;
}

bool string_eq(string a, string b) {
    return string_cmp(a,b) == 0;
}

string to_string(char* cstring) {
    return (string){cstring, strlen(cstring)};
}

// allocates and may return null
char* to_cstring(string str) {
    char* cstr = malloc(str.len + 1);
    memcpy(cstr, str.raw, str.len);
    cstr[str.len] = '\0';
    return cstr;
}

string string_alloc(size_t len) {
    char* raw = malloc(len);
    if (raw == NULL) return NULL_STR;

    return (string){raw, len};
}

void string_free(string str) {
    free(str.raw);
}
