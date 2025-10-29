#ifndef FE_MIR_XR_H
#define FE_MIR_XR_H

#include "../mir/mir.h"
#include <assert.h>

#define alignof_field(T, field) alignof(((T*)nullptr)->field)

typedef struct {
    u8 opcode;
    u8 ra;
    u8 rb;
    u8 rc;
    union {    
        u32 imm;
        struct {
            u8 shamt;
            u8 xsh;
            u8 funct;
        };
    };
} FeMirXrInst;
static_assert(alignof(FeMirXrInst) <= 4);

#endif // FE_MIR_H
