#pragma once
#define DEIMOS_TT_H

#include "orbit.h"
#include "mars.h"

void set_target_triple(string target, flag_set* fl);

#define SUPPORTED_ARCH \
    X(APHELION, "aphelion")

#define SUPPORTED_SYS \
    X(NONE, "none")
    
#define SUPPORTED_PRODUCT \
    X(ASM, "asm")

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