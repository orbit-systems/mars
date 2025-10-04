#ifndef FE_MIR_H
#define FE_MIR_H

#include "common/strmap.h"
#include "common/vec.h"

#include "iron/iron.h"
#include <assert.h>

typedef struct FeMirSection FeMirSection;
typedef struct {u32 _;} FeMirStringIndex;

typedef enum : u8 {
    FE_MIR_SYMKIND_NONE = 0,
    FE_MIR_SYMKIND_DATA,
    FE_MIR_SYMKIND_FUNCTION,
    
    // not associated with any section,
    // does not undergo relocation.
    FE_MIR_SYMKIND_ABSOLUTE,
} FeMirSymbolKind;

typedef enum : u8 {
    // not defined yet.
    FE_MIR_SYMBIND_EXTERN,

    FE_MIR_SYMBIND_PRIVATE,
    FE_MIR_SYMBIND_PUBLIC,
    FE_MIR_SYMBIND_PUBLIC_WEAK,
} FeMirSymbolBinding;

typedef struct {
    u16 section_index;
    FeMirSymbolKind kind;
    FeMirSymbolBinding binding;
    FeMirStringIndex name;
    u64 value; // offset from section it's defined in
} FeMirSymbol;

typedef enum : u8 {
    FE_MIR_SECTION_EXEC   = 1 << 0,
    FE_MIR_SECTION_READ   = 1 << 1,
    FE_MIR_SECTION_WRITE  = 1 << 2,

    FE_MIR_SECTION_BLANK  = 1 << 3,
    FE_MIR_SECTION_PINNED = 1 << 4,
} FeMirSectionFlags;

Vec_typedef(u8);

typedef struct FeMirSection {
    FeMirStringIndex name;
    FeMirSectionFlags flags;
    u16 alignment;

    u64 virt_addr;
} FeMirSection;
#define FE_MIR_NULL_SECTION 0

typedef enum : u8 {
    // an alignment directive.
    FE_MIR_ELEM_ALIGN,
    
    // a label that connects to a symbol
    FE_MIR_ELEM_LABEL,
    FE_MIR_ELEM_LOCAL_LABEL,

    // 
    FE_MIR_ELEM_RELOC,

    // a large length of bytes
    FE_MIR_ELEM_BYTES,

    FE_MIR_ELEM_D8,
    FE_MIR_ELEM_D16,
    FE_MIR_ELEM_D32,
    FE_MIR_ELEM_D64,

    // a block of adjacent target instructions,
    // allocated in the elem_extras buffer.
    FE_MIR_ELEM_INST_BLOCK,
} FeMirElementKind;

typedef struct FeMirElement {
    FeMirElementKind kind;

    // required byte alignment
    u8 byte_align;

    // size in bytes
    u16 byte_size;

    // byte offset from start of section
    u32 byte_offset;

    // 32 bits of whatever else is needed
    u32 opaque_extra;
} FeMirElement;

typedef struct FeMirElementInstBlock {
    u32 len;
    u32 insts[]; // need to cast to inst type first
} FeMirElementInstBlock;

typedef enum : u8 {
    MIR_RELOC__ADDEND,

    MIR_RELOC_PTR64,
    MIR_RELOC_PTR32,
    MIR_RELOC_PTR64_UNALIGNED,
    MIR_RELOC_PTR32_UNALIGNED,

    MIR_RELOC_XR_BRANCH,    // branch target
    MIR_RELOC_XR_ABSJ,      // XR absolute jump
    MIR_RELOC_XR_LA,        // XR LA pseudo-inst
    MIR_RELOC_XR_FAR_INT,   // XR far-int access psuedo-inst
    MIR_RELOC_XR_FAR_LONG,  // XR far-long access psuedo-inst
} FeMirRelocKind;

typedef struct {u32 _;} MirRelocIndex;
#define MIR_RELOC_NONE (MirRelocIndex){0}

typedef struct {
    FeMirRelocKind kind;
    bool uses_addend;
    u32 symbol_index;
} MirRelocation;

typedef struct {
    // shadow MirRelocation.kind for layout compatibility
    FeMirRelocKind _kind;
    u32 addend;
} MirRelocationAddend;
static_assert(sizeof(MirRelocation) == sizeof(MirRelocationAddend));
static_assert(alignof(MirRelocation) == alignof(MirRelocationAddend));

typedef enum : u8 {
    // standard object file
    FE_MIR_OBJ_RELOCATABLE,

    // shared library
    FE_MIR_OBJ_SHARED,

    // executable file
    FE_MIR_OBJ_EXECUTABLE,
} FeMirObjectKind;

Vec_typedef(FeMirSection);
Vec_typedef(FeMirSymbol);
Vec_typedef(u32);

#define FE_MIR_NULL_SYMBOL 0

typedef struct {
    FeMirObjectKind kind;
    FeArch arch;
    FeSystem system;

    Vec(u8) string_pool;
    Vec(FeMirSection) sections;
    Vec(FeMirSymbol) symbols;
    Vec(FeMirSymbol) relocations;

    Vec(u32) elem_extras;
    
    u32 entry_symbol; // only relevant for executable objects

    FeArena arena;
} FeMirObject;

FeMirObject* fe_mir_new_object(FeMirObjectKind kind);
FeMirSection* fe_mir_new_section(FeMirObject* obj, const char* name, u32 name_len, FeMirSectionFlags flags);

FeMirStringIndex fe_mir_intern_string(FeMirObject* obj, const char* name, u32 len);

#endif // FE_MIR_H
