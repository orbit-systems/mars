#ifndef FE_MIR_H
#define FE_MIR_H

#include "common/strmap.h"
#include "common/vec.h"

#include "iron/iron.h"
#include <assert.h>

typedef struct FeMirSection FeMirSection;
typedef struct FeMirRelocation FeMirRelocation;
typedef struct FeMirSymbol FeMirSymbol;
// type-safe buffer indices
typedef struct {u32 _;} FeMirSectionIndex;
typedef struct {u32 _;} FeMirStringIndex;
typedef struct {u32 _;} FeMirSymbolIndex;
typedef struct {u32 _;} FeMirRelocIndex;
typedef struct {u32 _;} FeMirExtraIndex;
#define FE_MIR_NULL_SECTION (FeMirSectionIndex){0}
#define FE_MIR_NULL_STRING  (FeMirStringIndex){0}
#define FE_MIR_NULL_SYMBOL  (FeMirSymbolIndex){0}
#define FE_MIR_NULL_RELOC   (FeMirRelocIndex){0}

Vec_typedef(FeMirSection);
Vec_typedef(FeMirRelocation);
Vec_typedef(FeMirSymbol);
Vec_typedef(u32);

typedef enum : u8 {
    FE_MIR_SYMKIND_NONE = 0,
    FE_MIR_SYMKIND_DATA,
    FE_MIR_SYMKIND_FUNCTION,
    
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

typedef struct FeMirSymbol {
    u16 section_index;
    FeMirSymbolKind kind;
    FeMirSymbolBinding binding;
    FeMirStringIndex name;
    u64 value; // offset from section it's defined in
} FeMirSymbol;

typedef enum : u8 {
    // an alignment directive.
    FE_MIR_ELEM_ALIGN,
    
    // a label that connects to a symbol
    FE_MIR_ELEM_LABEL,
    FE_MIR_ELEM_LOCAL_LABEL,

    // a relocation at this location.
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
    // this is filled in by the MIR builder
    // and should not be directy modified.
    u32 _byte_offset;

    // 32 bits of whatever else is needed
    union {
        FeMirExtraIndex index;
        FeMirStringIndex string;
        FeMirRelocIndex reloc;
        FeMirSymbolIndex symbol;
        u32 bits;
    } extra;
} FeMirElement;

typedef struct FeMirInstBlock {
    u32 len;
    u32 insts[]; // need to cast to inst type first
} FeMirInstBlock;

typedef enum : u8 {
    FE_MIR_SECTION_EXEC   = 1 << 0,
    FE_MIR_SECTION_READ   = 1 << 1,
    FE_MIR_SECTION_WRITE  = 1 << 2,

    FE_MIR_SECTION_BLANK  = 1 << 3,
} FeMirSectionFlags;

Vec_typedef(u8);
Vec_typedef(FeMirElement);

typedef struct FeMirSection {
    FeMirStringIndex name;
    FeMirSectionFlags flags;
    u16 alignment;

    Vec(FeMirElement) elems;

    u64 virt_addr;
} FeMirSection;

typedef enum : u8 {
    FE_MIR_RELOC__ADDEND,

    FE_MIR_RELOC_PTR64,
    FE_MIR_RELOC_PTR32,
    FE_MIR_RELOC_PTR64_UNALIGNED,
    FE_MIR_RELOC_PTR32_UNALIGNED,

    FE_MIR_RELOC_XR_BRANCH,     // branch target
    FE_MIR_RELOC_XR_ABSJ,       // XR absolute jump
    FE_MIR_RELOC_XR_LA,         // XR LA pseudo-inst
    FE_MIR_RELOC_XR_FAR_INT,    // XR far-int access psuedo-inst
    FE_MIR_RELOC_XR_FAR_LONG,   // XR far-long access psuedo-inst

    FE_MIR_RELOC__COUNT,
} FeMirRelocKind;

typedef struct FeMirRelocation {
    FeMirRelocKind kind;
    bool uses_addend;
    FeMirSymbolIndex symbol;
} FeMirRelocation;

typedef struct FeMirRelocationAddend {
    // shadow FeMirRelocation.kind for layout compatibility
    FeMirRelocKind _kind;
    u32 addend;
} FeMirRelocationAddend;
static_assert(sizeof(FeMirRelocation) == sizeof(FeMirRelocationAddend));
static_assert(alignof(FeMirRelocation) == alignof(FeMirRelocationAddend));

typedef enum : u8 {
    // standard object file
    FE_MIR_OBJ_RELOCATABLE,

    // shared library
    FE_MIR_OBJ_SHARED,

    // executable file
    FE_MIR_OBJ_EXECUTABLE,
} FeMirObjectKind;

typedef struct {
    FeMirObjectKind kind;
    FeArch arch;
    FeSystem system;

    Vec(u8) string_pool;
    Vec(FeMirSection) sections;
    Vec(FeMirSymbol) symbols;
    Vec(FeMirRelocation) relocations;

    Vec(u32) elem_extras;
    
    // only relevant for executable objects
    FeMirSymbolIndex entry_symbol;

    FeArena arena;
} FeMirObject;

FeMirObject* fe_mir_object_new(FeMirObjectKind kind);
FeMirSection* fe_mir_section_new(FeMirObject* obj, const char* name, u32 name_len, FeMirSectionFlags flags);

FeMirStringIndex fe_mir_intern_string(FeMirObject* obj, const char* name, u32 len);

// all of these may be invalidated when (obj) is modified in some way
void* fe_mir_extra_ptr(FeMirObject* obj, FeMirExtraIndex index);
const char* fe_mir_string_ptr(FeMirObject* obj, FeMirStringIndex index);
FeMirSymbol* fe_mir_symbol_ptr(FeMirObject* obj, FeMirSymbolIndex index);
FeMirRelocation* fe_mir_reloc_ptr(FeMirObject* obj, FeMirRelocIndex index);

FeMirElement* fe_mir_elem_new(FeMirObject* obj, FeMirSection* sec);
FeMirElement* fe_mir_elem_new(FeMirObject* obj, FeMirSection* sec);

#endif // FE_MIR_H
