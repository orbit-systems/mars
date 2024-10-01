#include "mach.h"

// "native" (pointer-sized) integer type for an architecture.
FeType fe_arch_type_of_native_int(u16 arch) {
    switch (arch) {
    case FE_ARCH_X64:      return FE_TYPE_I64;
    case FE_ARCH_APHELION: return FE_TYPE_I64;
    case FE_ARCH_ARM64:    return FE_TYPE_I64;
    case FE_ARCH_XR17032:  return FE_TYPE_I32;
    case FE_ARCH_FOX32:    return FE_TYPE_I32;
    default: CRASH("unknown arch");
    }
}

// highest precision floating point format less than or equal in size
// to the arch's native integer type
// returns FE_TYPE_VOID if there's no native floating-point
FeType fe_arch_type_of_native_float(u16 arch) {
    switch (arch) {
    case FE_ARCH_X64:      return FE_TYPE_F64;
    case FE_ARCH_APHELION: return FE_TYPE_F64;
    case FE_ARCH_ARM64:    return FE_TYPE_F64;
    case FE_ARCH_XR17032:  return FE_TYPE_VOID;
    case FE_ARCH_FOX32:    return FE_TYPE_VOID;
    default: CRASH("unknown arch");
    }
}

// returns true if this type has a "native" representation on this architecture
// ie. it does not need to be emulated via software or additional fuckery
// returns false for aggregate types
bool fe_arch_type_is_native(u16 arch, FeType t) {
    switch (arch) {
    case FE_ARCH_X64:
        // all types natively supported
        return t <= FE_TYPE_F64;
    case FE_ARCH_APHELION:
        // all types natively supported
        return t <= FE_TYPE_F64;
    case FE_ARCH_ARM64:
        // all types natively supported
        return t <= FE_TYPE_F64;
    case FE_ARCH_XR17032:
        // only integers up to i32 are native
        return t <= FE_TYPE_I32;
    case FE_ARCH_FOX32:
        // only integers up to i32 are native
        return t <= FE_TYPE_I32;
    default: CRASH("unknown arch");
    }
    return false;
}