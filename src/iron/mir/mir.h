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
    
    // symbol associated with a section,
    // not necessarily anything inside it.
    FE_MIR_SYMKIND_SECTION,

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

typedef enum : u16 {
    MIR_RELOC_EXTRA_ADDEND,

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

typedef struct {
    FeMirRelocKind kind;
    // offset into section.
    u32 offset;
    u32 symbol_index;
} FeMirReloc;

typedef union {
    FeMirRelocKind _kind; // shadow for layout
    u32 low;
    u32 high;
} FeMirRelocExtraAddend;
static_assert(sizeof(FeMirRelocExtraAddend) <= sizeof(FeMirReloc));
static_assert(alignof(FeMirRelocExtraAddend) <= alignof(FeMirReloc));

typedef enum : u8 {
    FE_MIR_SECTION_EXEC   = 1 << 0,
    FE_MIR_SECTION_READ   = 1 << 1,
    FE_MIR_SECTION_WRITE  = 1 << 2,

    FE_MIR_SECTION_BLANK  = 1 << 3,
} FeMirSectionFlags;

Vec_typedef(FeMirReloc);
Vec_typedef(u8);

typedef struct FeMirSection {
    FeMirStringIndex name;
    FeMirSectionFlags flags;
    u16 alignment;

    u64 virt_addr;

    Vec(FeMirReloc) relocs;
    Vec(u8) bytes;
} FeMirSection;
#define FE_MIR_NULL_SECTION 0

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

#define FE_MIR_NULL_SYMBOL 0

typedef struct {
    FeMirObjectKind kind;
    FeArch arch;
    FeSystem system;

    Vec(u8) string_pool;
    Vec(FeMirSection) sections;
    Vec(FeMirSymbol) symbols;
    // // when assembling, local symbols (not to be confused with private symbols)
    // // are kept in a small are at the top of the '.symbols' list. These are erased
    // // when a new non-local symbol is added.
    // u32 last_nonlocal_sym;
    
    u32 entry_symbol; // only relevant for executable objects

    FeArena arena;
} FeMirObject;

FeMirObject* fe_mir_new_object(FeMirObjectKind kind);
FeMirSection* fe_mir_new_section(FeMirObject* obj, const char* name, u32 name_len, FeMirSectionFlags flags);

FeMirStringIndex fe_mir_intern_string(FeMirObject* obj, const char* name, u32 len);

typedef struct {
    FeMirStringIndex name;
    // offset from last-defined symbol's value
    u32 offset;
} FeMirLocalLabel;

// if any local label fixups exist when a new nonlocal label is defined,
// something has gone wrong!
typedef struct {
    FeMirRelocKind kind;
    // offset into section.
    u32 offset;
} FeMirLocalLabelFixup;

Vec_typedef(FeMirLocalLabel);
Vec_typedef(FeMirLocalLabelFixup);

typedef struct {
    FeMirObject* obj;

    // current section
    FeMirSection* section;

    Vec(FeMirLocalLabel) local_labels;
    Vec(FeMirLocalLabelFixup) local_label_fixups;

    // index of last non_local symbol inside the current section.
    u32 last_nonlocal;

    u64 current_addr;

} FeMirAssemblerContext;

#endif // FE_MIR_H
