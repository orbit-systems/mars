#pragma once
#define STRSLICE_H

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

int    string_cmp(string a, string b);
bool   string_eq(string a, string b);
string to_string(char* cstring);
char*  to_cstring(string str); // this allocates

string string_alloc(size_t len);
#define string_free(str) free(str.raw)

string string_clone(string str); // this allocates as well