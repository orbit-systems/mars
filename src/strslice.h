#pragma once
#define STRSLICE_H

#include "orbit.h"

typedef struct string_s {
    char* raw;
    u32   len;
} string;

#define NULL_STR (string){NULL, 0}

#define string_make(ptr, len) (string){ptr,len}

#define string_len(s) (s.len)
#define string_raw(s) (s.raw)

#define is_substring(haystack, needle) (haystack.raw <= needle.raw) && (haystack.raw + haystack.len >= needle.raw + needle.len)
#define substring(str, start, end) (string){str.raw + start, end - start};

int    string_cmp(string a, string b);
bool   string_eq(string a, string b);
string to_string(char* cstring);
char*  to_cstring(string str); // this allocates

string string_alloc(size_t len);
void   string_free(string str);