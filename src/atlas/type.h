#pragma once
#define ATLAS_TYPE_H

#include "ir.h"


enum {
    AIR_NONE,

    AIR_BOOL,

    AIR_U8,
    AIR_U16,
    AIR_U32,
    AIR_U64,

    AIR_I8,
    AIR_I16,
    AIR_I32,
    AIR_I64,

    AIR_F16,
    AIR_F32,
    AIR_F64,

    AIR_POINTER,
    AIR_AGGREGATE,
    AIR_ARRAY,
};

typedef struct AIR_Type AIR_Type;

typedef struct AIR_Type {
    u8 kind;

    u32 align;
    u64 size;

    union {
    
        struct {
            u64 len;
            AIR_Type* fields[]; // flexible array member
        } aggregate;

        struct {
            u64 len;
            AIR_Type* sub;
        } array;
    
        AIR_Type* pointer;
    };
} AIR_Type;