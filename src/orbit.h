#pragma once
#define ORBIT_H

// standard Orbit Systems utility header

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
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

#define TODO(msg) \
    printf("TODO: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE)

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
