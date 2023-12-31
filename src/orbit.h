#pragma once
#define ORBIT_H

// standard Orbit Systems utility header

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <sys/time.h>


// not gonna use stdbool fuck you
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;
typedef uint8_t  bool;
#define false 0
#define true (!false)

#define TODO(msg) do {\
    printf("TODO: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE);} while (0)

#define CRASH(msg) do { \
    printf("CRASH: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE);} while (0)

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

#define FOR_RANGE_INCL(iterator, start, end) for (int iterator = (start); iterator <= (end); iterator++)
#define FOR_RANGE_EXCL(iterator, start, end) for (int iterator = (start); iterator < (end); iterator++)

// strings

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
void   printstr(string str);

string string_alloc(size_t len);
#define string_free(str) free(str.raw)

string string_clone(string str); // this allocates as well