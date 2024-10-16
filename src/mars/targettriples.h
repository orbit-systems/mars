#pragma once
#define MARS_TT_H

#include "common/orbit.h"
#include "mars/mars.h"

void set_target_triple(string target, flag_set* fl);

#define SUPPORTED_ARCH      \
    X(APHELION, "aphelion") \
    X(X86_64, "x86_64")

#define SUPPORTED_SYS  \
    X(NONE, "unknown") \
    X(LINUX, "linux")

#define SUPPORTED_PRODUCT \
    X(ASM, "asm")         \
    X(ELF, "elf")

enum {
#define X(variant, str) TARGET_ARCH_##variant,
    SUPPORTED_ARCH
#undef X
};

enum {
#define X(variant, str) TARGET_SYS_##variant,
    SUPPORTED_SYS
#undef X
};

enum {
#define X(variant, str) TARGET_PRODUCT_##variant,
    SUPPORTED_PRODUCT
#undef X
};