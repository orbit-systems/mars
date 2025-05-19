#pragma once
#define ORBIT_TYPES_H

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;

typedef intptr_t  isize;
typedef uintptr_t usize;

typedef double   f64;
typedef float    f32;
typedef _Float16 f16;

#if __STDC_VERSION__ < 202311L
#if __STDC_VERSION >= 199901L
#include <stdbool.h>
#else
typedef uint8_t bool;
#define false ((bool)0)
#define true ((bool)1)
#endif
#endif
