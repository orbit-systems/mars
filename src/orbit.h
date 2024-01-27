#pragma once
#define ORBIT_H

// orbit systems utility header
// this is prolly public domain (CC0), idk tho

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <sys/time.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;
typedef uint8_t  bool;
typedef float f32;
typedef double f64;
#define false 0
#define true (!false)

#define TODO(msg) do {\
    printf("\x1b[36m\x1b[1mTODO\x1b[0m: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE); } while (0)

#define CRASH(msg) do { \
    printf("\x1b[31m\x1b[1mCRASH\x1b[0m: \"%s\" at %s:%d\n", (msg), (__FILE__), (__LINE__)); \
    exit(EXIT_FAILURE); } while (0)

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

#define FOR_RANGE_INCL(iterator, start, end) for (intptr_t iterator = (start); iterator <= (end); iterator++)
#define FOR_RANGE(iterator, start, end) for (intptr_t iterator = (start); iterator < (end); iterator++)

#define FOR_URANGE_INCL(iterator, start, end) for (uintptr_t iterator = (start); iterator <= (end); iterator++)
#define FOR_URANGE(iterator, start, end) for (uintptr_t iterator = (start); iterator < (end); iterator++)

#define is_pow_2(i) ((i & (i-1)) == 0)

#include "orbitstr.h"
#include "orbitfs.h"