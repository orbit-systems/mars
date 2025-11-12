#ifndef IRON_H
#define IRON_H

#define FE_VERSION_MAJOR 0
#define FE_VERSION_MINOR 1
#define FE_VERSION (FE_VERSION_MAJOR * 100 + FE_VERSION_MINOR)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdalign.h>

#if __STDC_VERSION__ <= 201710L 
    #error Iron is a C23 library!
#endif

#if defined(__x86_64__)
    #define FE_HOST_X64
    #define FE_HOST_BITS 64
#elif defined (__i386__)
    #define FE_HOST_X86
    #define FE_HOST_BITS 32
#elif defined (__aarch64__)
    #define FE_HOST_ARM64
    #define FE_HOST_BITS 64
#elif defined (__arm__)
    #define FE_HOST_ARM32
    #define FE_HOST_BITS 32
#else
    #error unrecognized host!
#endif

// -------------------------------------
// simple utilities
// -------------------------------------

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
#define USIZE_MAX UINTPTR_MAX
#define ISIZE_MAX INTPTR_MAX

typedef double   f64;
typedef float    f32;

#ifdef FE_HOST_X64
    typedef struct FeCompactStr {
        i64 data : 48;
        u64 len : 16;
    } FeCompactStr;
    #define fe_compstr(data_, len_) ((FeCompactStr){.data = (i64)(char*)(data_), .len = (len_)})
    #define fe_compstr_data(compstr) ((char*)(i64)(compstr).data)
    #define fe_compstr_fmt "%.*s"
    #define fe_compstr_arg(compstr) (compstr).data, (char*)(i64)(compstr).data
#else
    // fallback to regular string
    typedef struct FeCompactStr {
        char* data;
        u16 len;
    } FeCompactStr;
    #define fe_compstr(data_, len_) ((FeCompactStr){.data = (data_), .len = (len_)})
    #define fe_compstr_data(compstr) ((compstr).data)
#endif

// feel free to make iron use your own heap-like allocator.
// only works when statically linking.
#ifndef FE_CUSTOM_ALLOCATOR
    #define fe_malloc  malloc
    #define fe_realloc realloc
    #define fe_free    free
#else
    void* fe_malloc(usize size);
    void* fe_realloc(void* ptr, usize size);
    void fe_free(void* ptr);
#endif

#define fe_zalloc(size) memset(fe_malloc(size), 0, size);

// simple ++n loop
#ifndef for_n
    #define for_n(iterator, start, end) for (usize iterator = (start); iterator < (end); ++iterator)
#endif

#define FE_CRASH(fmt, ...) fe_runtime_crash("crash in %s() at %s:%zu -> " fmt, __func__, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define FE_ASSERT(cond) if (!(cond)) fe_runtime_crash("FE_ASSERT(" #cond ") failed in %s() at %s:%zu", __func__, __FILE__, __LINE__)

// -------------------------------------
// predefs!
// -------------------------------------

typedef struct FeInst FeInst;
typedef struct FeBlock FeBlock;
typedef struct FeSymbol FeSymbol;
typedef struct FeFunc FeFunc;
typedef struct FeFuncSig FeFuncSig;
typedef struct FeSection FeSection;
typedef struct FeModule FeModule;
typedef struct FeTarget FeTarget;
typedef struct FeStackItem FeStackItem;
typedef struct FeInstPool FeInstPool;
typedef struct FeArena FeArena;

typedef u32 FeVReg; // vreg index
typedef struct FeVRegBuffer FeVRegBuffer;
typedef struct FeBlockLiveness FeBlockLiveness;

// -------------------------------------
// internal type system
// -------------------------------------

#define FE_TY_VEC(vecty, elemty) (vecty + elemty)
#define FE_TY_VEC_ELEM(ty) (ty & 0b1111)
#define FE_TY_VEC_SIZETY(ty) (ty & 0b110000)
#define FE_TY_IS_VEC(ty) (ty >= FE_TY_V128)

typedef enum: u8 {
    FE_TY_VOID,

    FE_TY_BOOL,

    FE_TY_I8,
    FE_TY_I16,
    FE_TY_I32,
    FE_TY_I64,

    FE_TY_F32,
    FE_TY_F64,

    // member types dependent on source inst
    FE_TY_TUPLE,

    // there's an FeRecord associated with this somewhere near
    FE_TY_RECORD,
    FE_TY_ARRAY,

    FE_TY_V128 = 0b010000, // elem type fits in lower 4 bits
    FE_TY_V256 = 0b100000,
    FE_TY_V512 = 0b110000,

    FE_TY_I8x16 = FE_TY_VEC(FE_TY_V128, FE_TY_I8),
    FE_TY_I16x8 = FE_TY_VEC(FE_TY_V128, FE_TY_I16),
    FE_TY_I32x4 = FE_TY_VEC(FE_TY_V128, FE_TY_I32),
    FE_TY_I64x2 = FE_TY_VEC(FE_TY_V128, FE_TY_I64),
    FE_TY_F32x4 = FE_TY_VEC(FE_TY_V128, FE_TY_F32),
    FE_TY_F64x2 = FE_TY_VEC(FE_TY_V128, FE_TY_F64),

    FE_TY_I8x32  = FE_TY_VEC(FE_TY_V256, FE_TY_I8),
    FE_TY_I16x16 = FE_TY_VEC(FE_TY_V256, FE_TY_I16),
    FE_TY_I32x8  = FE_TY_VEC(FE_TY_V256, FE_TY_I32),
    FE_TY_I64x4  = FE_TY_VEC(FE_TY_V256, FE_TY_I64),
    FE_TY_F32x8  = FE_TY_VEC(FE_TY_V256, FE_TY_F32),
    FE_TY_F64x4  = FE_TY_VEC(FE_TY_V256, FE_TY_F64),
    
    FE_TY_I8x64  = FE_TY_VEC(FE_TY_V512, FE_TY_I8),
    FE_TY_I16x32 = FE_TY_VEC(FE_TY_V512, FE_TY_I16),
    FE_TY_I32x16 = FE_TY_VEC(FE_TY_V512, FE_TY_I32),
    FE_TY_I64x8  = FE_TY_VEC(FE_TY_V512, FE_TY_I64),
    FE_TY_F32x16 = FE_TY_VEC(FE_TY_V512, FE_TY_F32),
    FE_TY_F64x8  = FE_TY_VEC(FE_TY_V512, FE_TY_F64),

    FE__TY_END,
} FeTy;

typedef struct FeComplexTy FeComplexTy;
typedef struct FeRecordField FeRecordField;

// for representing struct (record) and array types
typedef struct FeComplexTy {
    union {
        FeTy ty;
        struct {
            FeTy _ty; // shadow of ty for layout
            FeTy elem_ty;
            u32 len;
            FeComplexTy* complex_elem_ty;
        } array;
        struct {
            FeTy _ty; // shadow of ty for layout
            u32 fields_len;
            FeRecordField* fields;
        } record;
    };
} FeComplexTy;

typedef struct FeRecordField {
    FeTy ty;
    u16 offset;
    FeComplexTy* complex_ty; // if applicable
} FeRecordField;

// if ty is not a complex type, cty should be nullptr
usize fe_ty_get_size(FeTy ty, FeComplexTy* cty);
usize fe_ty_get_align(FeTy ty, FeComplexTy* cty);

// -------------------------------------
// architecture/ABI stuff
// -------------------------------------

typedef enum FeArch : u8 {
    FE_ARCH_X64 = 1,
    FE_ARCH_ARM64,

    FE_ARCH_XR17032,
    // FE_ARCH_FOX32,
    // FE_ARCH_APHELION,
} FeArch;

typedef enum FeSystem : u8 {
    FE_SYSTEM_FREESTANDING = 1,
} FeSystem;

typedef enum FeCallConv : u8 {
    // pass parameters however iron sees fit
    // may not have stable ABI, dangerous for
    // public/extern functions
    FE_CCONV_ANY = 0,

    // os specific
        // standard linux c callconv
        FE_CCONV_SYSV,
        // standard windows c callconv
        FE_CCONV_STDCALL,

    // language-specific
        // standard c calling convention on target
        FE_CCONV_C,
        // jackal calling convention on target
        FE_CCONV_JACKAL,
} FeCallConv;

// -------------------------------------
// Iron IR
// -------------------------------------

typedef enum FeSymbolBinding : u8 {
    FE_BIND_LOCAL,
    FE_BIND_GLOBAL,
    FE_BIND_WEAK,

    // shared libary stuff
    FE_BIND_SHARED_EXPORT,
    FE_BIND_SHARED_IMPORT,

    FE_BIND_EXTERN,
} FeSymbolBinding;

typedef enum FeSymbolKind : u8 {
    FE_SYMKIND_NONE = 0,
    FE_SYMKIND_FUNC,
    FE_SYMKIND_DATA,
} FeSymbolKind;

typedef struct FeSymbol {
    FeCompactStr name;
    FeSection* section;

    FeSymbolKind kind;    // what does this symbol refer to?
    FeSymbolBinding bind; // where is this symbol visible?

    union {
        FeFunc* func;
    };
} FeSymbol;

typedef struct FeFuncParam {
    FeTy ty;
    FeComplexTy* cty;
} FeFuncParam;

// holds type/interface information about a function.

typedef struct FeFuncSig {
    FeCallConv cconv;
    u16 param_len;
    u16 return_len;
    FeFuncParam params[];
} FeFuncSig;

typedef struct FeFunc {
    FeSymbol* sym;
    FeFuncSig* sig;
    FeModule* mod;

    u32 id_count;
    u32 block_count;
    FeInstPool* ipool;
    FeVRegBuffer* vregs;

    FeInst** params; // gets length of params from signature

    FeBlock* entry_block;
    FeBlock* last_block;
    
    FeFunc* list_next;
    FeFunc* list_prev;

    FeStackItem* stack_top; // most-positive offset from stack pointer
    FeStackItem* stack_bottom;
} FeFunc;

typedef struct FeStaticData {
    FeSymbol* sym;
    FeComplexTy* ty; 
} FeStaticData;

typedef struct FeSymTab {
    struct {
        FeCompactStr name;
        FeSymbol* sym;
    }* entries;
    u32 cap;
} FeSymTab;

void fe_symtab_init(FeSymTab* st);
void fe_symtab_put(FeSymTab* st, FeSymbol* sym);
FeSymbol* fe_symtab_get(FeSymTab* st, const char* data, u16 len);
void fe_symtab_remove(FeSymTab* st, const char* data, u16 len);
void fe_symtab_destroy(FeSymTab* st);

typedef enum FeSectionFlags : u8 {
    FE_SECTION_WRITEABLE   = 1 << 0,
    FE_SECTION_EXECUTABLE  = 1 << 1,
    FE_SECTION_THREADLOCAL = 1 << 2,
    FE_SECTION_COMMON      = 1 << 3,
} FeSectionFlags;

typedef struct FeSection {
    FeCompactStr name;
    FeSectionFlags flags;

    FeSection* next;
    FeSection* prev;
} FeSection;

FeSection* fe_section_new(FeModule* m, const char* name, u16 len, FeSectionFlags flags);

typedef struct FeModule {

    struct {
        FeSection* first;
        FeSection* last;
    } sections;

    struct {
        FeFunc* first;
        FeFunc* last;
    } funcs;

    const FeTarget* target;

    FeSymTab symtab;
} FeModule;

typedef struct FeBlock {
    u32 flags;
    u32 id;
    u16 pred_len;
    u16 pred_cap;
    u16 succ_len;
    u16 succ_cap;

    FeFunc* func;
    FeInst* bookend;

    FeBlock* list_next;
    FeBlock* list_prev;

    FeBlock** pred;
    FeBlock** succ;

    // liveness info
    FeBlockLiveness* live;
} FeBlock;

#define for_funcs(f, modptr) \
    for (FeFunc* f = (modptr)->funcs.first; f != nullptr; f = f->list_next)

#define for_blocks(block, funcptr) \
    for (FeBlock* block = (funcptr)->entry_block; block != nullptr; block = block->list_next)

#define for_inst(inst, blockptr) \
    for (FeInst* inst = (blockptr)->bookend->next, *_next_ = inst->next; inst->kind != FE__BOOKEND; inst = _next_, _next_ = _next_->next)

#define for_inst_reverse(inst, blockptr) \
    for (FeInst* inst = (blockptr)->bookend->prev, *_prev_ = inst->prev; inst->kind != FE__BOOKEND; inst = _prev_, _prev_ = _prev_->prev)

typedef u16 FeInstKind;
typedef enum FeInstKindGeneric : FeInstKind {
    // Bookend
    FE__BOOKEND = 1,

    // FeInstParam
    FE__ROOT, // root instruction

    // FeInstProj
    // {tuple}
    FE_PROJ,

    // FeInstConst
    FE_CONST,

    // FeInstSymAddr
    FE_SYM_ADDR,
    // FeInstStack
    FE_STACK_ADDR,

    // {lhs, rhs}
    FE_IADD,
    FE_ISUB,
    FE_IMUL, 
    FE_IDIV, FE_UDIV,
    FE_IREM, FE_UREM,
    FE_AND,
    FE_OR,
    FE_XOR,
    FE_SHL,
    FE_USR, FE_ISR,
    
    FE_ILT, FE_ULT,
    FE_ILE, FE_ULE,
    FE_IEQ,
    
    FE_FADD,
    FE_FSUB,
    FE_FMUL,
    FE_FDIV,
    FE_FREM,

    // {src}
    FE_MOV,
    FE__MACH_MOV, // mostly for hardcoding register clobbers
    FE__MACH_UPSILON, // for phi-movs in codegen

    FE_TRUNC,    // integer downcast
    FE_SIGN_EXT,  // signed integer upcast
    FE_ZERO_EXT,  // unsigned integer upcast
    FE_BITCAST,
    FE_I2F, // integer to float
    FE_F2I, // float to integer
    FE_U2F, // unsigned integer to float
    FE_F2U, // float to unsigned nteger

    // FeInstMemop
    // {last_mem, ptr}
    FE_LOAD,

    // FeInstMemop
    // {last_mem, ptr, val}
    FE_STORE,

    // (void)
    FE_MEM_BARRIER,

    // (void)
    // terminator inst that assumes control flow cannot reach it
    // optimizations may do with this what they want
    FE_UNREACHABLE,

    // FeInstBranch
    // {val}
    FE_BRANCH,

    // FeInstJump
    FE_JUMP,

    // FeInstReturn
    // {last_mem, src1, src2, ...}
    FE_RETURN,

    // FeInstPhi
    // {src1, src2, ...}
    FE_PHI,
    FE_MEM_PHI,

    // void
    // {last_mem1, last_mem2, ...}
    FE_MEM_MERGE,

    // FeInstCall
    // {last_mem, ptr, src1, src2, ...}
    FE_CALL,

    // FeInstInlineAsm
    // {?last_mem, in1, in2, ...} -> {out1, out2, ...}
    FE_INLINE_ASM,

    // (void)
    FE__MACH_REG,
    FE__MACH_RETURN, // signals that the final instruction returns, even if its not a terminator

    // FeInstStack
    FE__MACH_STACK_SPILL,
    // FeInstStack
    FE__MACH_STACK_RELOAD,

    FE__BASE_INST_END,

    FE__X64_INST_BEGIN = FE_ARCH_X64 * 128,
    FE__X64_INST_END = FE__X64_INST_BEGIN + 128,

    FE__XR_INST_BEGIN = FE_ARCH_XR17032 * 128,
    FE__XR_INST_END = FE__XR_INST_BEGIN + 128,

    FE__INST_END,
} FeInstKindGeneric;

// if possible, bit-pack uses
#ifdef FE_HOST_X64
    typedef struct FeInstUse {
        u64 idx : 16; // REALLY shouldn't need more than this...
        i64 ptr : 48;
    } FeInstUse;
    static_assert(sizeof(FeInstUse) == 8);
    #define FE_USE_PTR(use) ((FeInst*)(i64)(use).ptr)
#else
    typedef struct FeInstUse {
        u16 idx;
        FeInst* ptr;
    } FeInstUse;
    #define FE_USE_PTR(use) (use).ptr
#endif

typedef struct FeInst {
    FeInstKind kind;
    FeTy ty;
    
    u16 use_len;
    u16 use_cap;
    u16 in_len;
    u16 in_cap;

    u32 flags;
    u32 id;
    FeVReg vr_def;
    
    FeInstUse* uses;
    FeInst** inputs;

    // CIRCULAR
    FeInst* prev;
    FeInst* next;

    usize extra[];
} FeInst;

typedef enum FeInlineAsmParamKind : u8 {
    FE_ASM_SCRATCH = 0b00, // used when an register needs to be picked but is neither IN nor OUT
    FE_ASM_IN      = 0b01,
    FE_ASM_OUT     = 0b10,
    FE_ASM_INOUT   = 0b11,
} FeInlineAsmParamKind;

typedef struct FeInlineAsmParam {
    FeTy ty; // target chooses register class from provided type
    FeInlineAsmParamKind kind;
} FeInlineAsmParam;

typedef struct FeInstInlineAsm {
    /*
        a: in, b: out, c: inout, d: inout, e: in, f: out
    ==
        (a, c, d, e) -> (c, d, f)  
    */
    u16 num_params;
    bool mem_use;
    bool mem_def;

    /* regular assembly text (no labels allowed)
        - use `#{N}` to designate spots for registers
    */
    FeCompactStr assembly;

    FeInlineAsmParam params[];
} FeInstInlineAsm;

typedef struct FeInlineAsmAnalysis {
    // analysis
    bool mem_def : 1;
    bool mem_use : 1;
    bool may_diverge : 1;

    // error reporting
    bool error : 1;
    FeCompactStr error_snippet;
    FeCompactStr error_message;
} FeInlineAsmAnalysis;

FeInlineAsmAnalysis fe_inline_asm_analyze(FeFunc* f, FeBlock* b, FeInst* inline_asm);

typedef struct {
    FeBlock* block;
} FeInst_Bookend;

typedef struct {
    usize index;
} FeInstProj;

typedef union {
    u64 val;
    f64 val_f64;
    f32 val_f32;
} FeInstConst;

typedef struct {
    FeStackItem* item;
} FeInstStack;

typedef struct {
    FeSymbol* sym;
} FeInstSymAddr;

#define FE_MEMOP_ALIGN_DEFAULT 0
typedef struct {
    // must be a power of two.
    u16 align;
    // amount that the provided pointer is offset from
    // the provided alignment
    u8  offset;
} FeInstMemop;

typedef struct {
    FeBlock* if_true;
    FeBlock* if_false;
} FeInstBranch;

typedef struct {
    FeBlock* to;
} FeInstJump;

typedef struct {
    FeBlock** blocks;
} FeInstPhi;

typedef struct {
    FeFuncSig* sig;
} FeInstCall;

#define FE_STACK_OFFSET_UNDEF UINT32_MAX

typedef struct FeStackItem {
    FeStackItem* next; // points to top
    FeStackItem* prev; // points to bottom

    FeComplexTy* complex_ty;
    FeTy ty;

    u16 flags;

    u32 _offset;
} FeStackItem;

FeStackItem* fe_stack_item_new(FeTy ty, FeComplexTy* cty);
FeStackItem* fe_stack_append_bottom(FeFunc* f, FeStackItem* item);
FeStackItem* fe_stack_append_top(FeFunc* f, FeStackItem* item);
FeStackItem* fe_stack_remove(FeFunc* f, FeStackItem* item);
u32 fe_stack_calculate_size(FeFunc* f);

typedef enum FeTrait : u32 {
    // x op y == y op x
    FE_TRAIT_COMMUTATIVE      = 1u << 0,
    // (x op y) op z == x op (y op z)
    FE_TRAIT_ASSOCIATIVE      = 1u << 1,
    // (x op y) op z == x op (y op z) trust me vro 💀🙏🥀
    // (only associative under imprecise 'fast' circumstances)
    FE_TRAIT_FAST_ASSOCIATIVE = 1u << 2,
    // cannot be simply removed if result is not used
    FE_TRAIT_VOLATILE         = 1u << 3,
    // can only terminate a basic block
    FE_TRAIT_TERMINATOR       = 1u << 4,
    // output type = first input type
    FE_TRAIT_SAME_IN_OUT_TY   = 1u << 5,
    // all input types must be the same type  
    FE_TRAIT_SAME_INPUT_TYS   = 1u << 6,
    // all input types must be integers
    FE_TRAIT_INT_INPUT_TYS    = 1u << 7,
    // all input types must be floats
    FE_TRAIT_FLT_INPUT_TYS    = 1u << 8,
    // allow vector types
    FE_TRAIT_VEC_INPUT_TYS    = 1u << 9,
    // output type = bool
    FE_TRAIT_BOOL_OUT_TY      = 1u << 10,
    // reg->reg move; allocator should hint
    // the output and input to be the same
    FE_TRAIT_REG_MOV_HINT     = 1u << 11,
    // is an algebraic binary operation
    FE_TRAIT_BINOP            = 1u << 12,
    // is an algebraic unary operation
    FE_TRAIT_UNOP             = 1u << 13,
    // creates a memory effect
    FE_TRAIT_MEM_DEF          = 1u << 14,
    // first input is a memory effect
    FE_TRAIT_MEM_SINGLE_USE   = 1u << 15,
    // ALL inputs are memory effects
    FE_TRAIT_MEM_MULTI_USE    = 1u << 16,
} FeTrait;

FeTrait fe_inst_traits(FeInstKind kind);
bool fe_inst_has_trait(FeInstKind kind, FeTrait trait);
void fe__load_trait_table(usize start_index, const FeTrait* table, usize len);

usize fe_inst_extra_size(FeInstKind kind);
usize fe_inst_extra_size_unsafe(FeInstKind kind);
void fe__load_extra_size_table(usize start_index, const u8* table, usize len);

FeModule* fe_module_new(FeArch arch, FeSystem system);
void fe_module_destroy(FeModule* mod);

FeSymbol* fe_symbol_new(FeModule* m, const char* name, u16 len, FeSection* section, FeSymbolBinding bind);
void fe_symbol_destroy(FeSymbol* sym);

FeFuncSig* fe_funcsig_new(FeCallConv cconv, u16 param_len, u16 return_len);
FeFuncParam* fe_funcsig_param(FeFuncSig* sig, u16 index);
FeFuncParam* fe_funcsig_return(FeFuncSig* sig, u16 index);
void fe_funcsig_destroy(FeFuncSig* sig);

FeBlock* fe_block_new(FeFunc* f);
void fe_block_destroy(FeBlock* block);

FeFunc* fe_func_new(
    FeModule* mod,
    FeSymbol* sym,
    FeFuncSig* sig,
    FeInstPool* ipool,
    FeVRegBuffer* vregs);
void fe_func_destroy(FeFunc* f);
FeInst* fe_func_param(FeFunc* f, u16 index);

FeInst* fe_insert_before(FeInst* point, FeInst* i);
FeInst* fe_insert_after(FeInst* point, FeInst* i);
#define fe_append_begin(block, inst) fe_insert_after((block)->bookend, inst)
#define fe_append_end(block, inst) fe_insert_before((block)->bookend, inst)
FeInst* fe_inst_remove_from_block(FeInst* inst);
void fe_inst_replace_pos(FeInst* from, FeInst* to);

// might need to allocate
void fe_cfg_add_edge(FeFunc* f, FeBlock* pred, FeBlock* succ);
void fe_cfg_remove_edge(FeBlock* pred, FeBlock* succ);

// -------------------------------------
// worklist
// -------------------------------------

typedef struct FeCompactMap {
    uintptr_t* values;
    usize* exists;
    u32 id_start;
    u32 id_end;
} FeCompactMap;

void fe_cmap_init(FeCompactMap* cmap);
void fe_cmap_destroy(FeCompactMap* cmap);
void fe_cmap_put(FeCompactMap* cmap, u32 key, uintptr_t val);
uintptr_t* fe_cmap_get(FeCompactMap* cmap, u32 key);
uintptr_t fe_cmap_pop_next(FeCompactMap* cmap);
bool fe_cmap_contains_key(FeCompactMap* cmap, u32 key);
void fe_cmap_remove(FeCompactMap* cmap, u32 key);

typedef struct FeInstSet {
    FeCompactMap _;
} FeInstSet;

static inline void fe_iset_init(FeInstSet* iset) {
    fe_cmap_init(&iset->_);
}

static inline void fe_iset_destroy(FeInstSet* iset) {
    fe_cmap_destroy(&iset->_);
}

static inline void fe_iset_push(FeInstSet* iset, FeInst* inst) {
    fe_cmap_put(&iset->_, inst->id, (uintptr_t)inst);
}

static inline FeInst* fe_iset_pop(FeInstSet* iset) {
    return (FeInst*) fe_cmap_pop_next(&iset->_);
}

static inline bool fe_iset_contains(FeInstSet* iset, FeInst* inst) {
    return fe_cmap_contains_key(&iset->_, inst->id);
}

static inline void fe_iset_remove(FeInstSet* iset, FeInst* inst) {
    fe_cmap_remove(&iset->_, inst->id);
}

// -------------------------------------
// aliasing
// -------------------------------------

typedef struct FeAliasMap {
    FeCompactMap cmap;
    usize num_cat;
} FeAliasMap;

typedef struct FeAliasCategory {
    u32 _;
} FeAliasCategory;
static_assert(sizeof(FeAliasCategory) <= sizeof(uintptr_t));

void fe_amap_init(FeAliasMap* amap);
void fe_amap_destroy(FeAliasMap* amap);
FeAliasCategory fe_amap_new_category(FeAliasMap* amap);
void fe_amap_add(FeAliasMap* amap, u32 inst_id, FeAliasCategory cat);
usize fe_amap_get_len(FeAliasMap* amap, u32 inst_id);
FeAliasCategory* fe_amap_get(FeAliasMap* amap, u32 inst_id);

void fe_solve_mem(FeFunc* f, FeAliasMap* amap);
void fe_solve_mem_root(FeFunc* f);
void fe_solve_mem_pessimistic(FeFunc* f);
bool fe_insts_may_alias(FeFunc* f, FeInst* i1, FeInst* i2);

// -------------------------------------
// instruction chains
// -------------------------------------

// a "chain" is a free-floating (not attached to a block)
// list of instructions.
typedef struct FeInstChain {
    FeInst* begin;
    FeInst* end;
} FeInstChain;

#define FE_EMPTY_CHAIN (FeInstChain){nullptr, nullptr}

FeInstChain fe_chain_new(FeInst* initial);
FeInstChain fe_chain_append_end(FeInstChain chain, FeInst* i);
FeInstChain fe_chain_append_begin(FeInstChain chain, FeInst* i);
FeInstChain fe_chain_concat(FeInstChain front, FeInstChain back);
void fe_insert_chain_before(FeInst* point, FeInstChain chain);
void fe_insert_chain_after(FeInst* point, FeInstChain chain);
void fe_chain_replace_pos(FeInst* from, FeInstChain to);
void fe_chain_destroy(FeFunc* f, FeInstChain chain);

// -------------------------------------
// instruction builders
// -------------------------------------

usize fe_replace_uses(FeFunc* f, FeInst* old_val, FeInst* new_val);
void fe_set_input(FeFunc* f, FeInst* inst, u16 n, FeInst* input);
void fe_set_input_null(FeInst* inst, u16 n);
void fe_add_input(FeFunc* f, FeInst* inst, FeInst* input);

// not really for external use
FeInst* fe_inst_new(FeFunc* f, usize input_len, usize extra_size);
void fe_inst_destroy(FeFunc* f, FeInst* inst);

FeInst* fe_inst_proj(FeFunc* f, FeInst* inst, usize index);
FeInst* fe_inst_const(FeFunc* f, FeTy ty, u64 val);
FeInst* fe_inst_const_f64(FeFunc* f, f64 val);
FeInst* fe_inst_const_f32(FeFunc* f, f32 val);
FeInst* fe_inst_stack_addr(FeFunc* f, FeStackItem* item);
FeInst* fe_inst_sym_addr(FeFunc* f, FeSymbol* sym);
FeInst* fe_inst_unop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* val);
FeInst* fe_inst_binop(FeFunc* f, FeTy ty, FeInstKind kind, FeInst* lhs, FeInst* rhs);
FeInst* fe_inst_load(FeFunc* f, FeTy ty, FeInst* ptr, u16 align, u8 offset);
FeInst* fe_inst_store(FeFunc* f, FeInst* ptr, FeInst* val, u16 align, u8 offset);
FeInst* fe_inst_call(FeFunc* f, FeInst* callee, FeFuncSig* sig);
FeInst* fe_inst_return(FeFunc* f);
FeInst* fe_inst_branch(FeFunc* f, FeInst* cond);
FeInst* fe_inst_jump(FeFunc* f);
FeInst* fe_inst_phi(FeFunc* f, FeTy ty, u16 expected_len);
FeInst* fe_inst_mem_phi(FeFunc* f, u16 expected_len);
FeInst* fe_inst_mem_merge(FeFunc* f, u16 expected_len);
FeInst* fe_inst_mem_barrier(FeFunc* f);

void fe_call_set_arg(FeFunc* f, FeInst* call, u16 n, FeInst* arg) ;
void fe_return_set_arg(FeFunc* f, FeInst* ret, u16 n, FeInst* arg);
void fe_branch_set_true(FeFunc* f, FeInst* branch, FeBlock* block);
void fe_branch_set_false(FeFunc* f, FeInst* branch, FeBlock* block);
void fe_jump_set_target(FeFunc* f, FeInst* jump, FeBlock* block);
void fe_phi_add_src(FeFunc* f, FeInst* phi, FeInst* src_value, FeBlock* src_block);
void fe_phi_remove_src(FeFunc* f, FeInst* phi, u16 n);

const char* fe_inst_name(const FeTarget* target, FeInstKind kind);
const char* fe_ty_name(FeTy ty);

FeBlock** fe_list_terminator_successors(const FeTarget* t, FeInst* term, usize* len_out);

FeTy fe_proj_ty(FeInst* tuple, usize index);

#define fe_extra(instptr, ...) (__VA_OPT__((__VA_ARGS__*))(void*)&(instptr)->extra[0])

#define fe_from_extra(extraptr) ((FeInst*)((usize)extraptr - offsetof(FeInst, extra)))

// check this assumption with some sort
// with a runtime init function
#define FE__INST_EXTRA_MAX_SIZE sizeof(FeInstBranch)

// -------------------------------------
// allocation
// -------------------------------------

#define FE__IPOOL_INST_FREE_SPACES_LEN (FE__INST_EXTRA_MAX_SIZE / sizeof(usize) + 1)
typedef struct Fe__InstPoolChunk Fe__InstPoolChunk;
typedef struct Fe__InstPoolFreeSpace Fe__InstPoolFreeSpace;
typedef struct FeInstPool {
    Fe__InstPoolChunk* top;
    Fe__InstPoolFreeSpace* inst_free_spaces[FE__INST_EXTRA_MAX_SIZE];

    Fe__InstPoolFreeSpace* lists_pow_2[10]; // 1, 2, 4, 8, 16, 32, 64, 128, 256, 512
} FeInstPool;

void fe_ipool_init(FeInstPool* pool);
FeInst* fe_ipool_alloc(FeInstPool* pool, usize extra_size);
void fe_ipool_free(FeInstPool* pool, FeInst* inst);
usize fe_ipool_free_manual(FeInstPool* pool, FeInst* inst);
void fe_ipool_destroy(FeInstPool* pool);

// list allocation!
void* fe_ipool_list_alloc(FeInstPool* pool, usize list_cap);
void fe_ipool_list_free(FeInstPool* pool, void* list, usize list_cap);

typedef struct Fe__ArenaChunk Fe__ArenaChunk;
typedef struct FeArena {
    Fe__ArenaChunk* top;
} FeArena;

typedef struct FeArenaState {
    Fe__ArenaChunk* top;
    usize used;
} FeArenaState;

void fe_arena_init(FeArena* arena);
void fe_arena_destroy(FeArena* arena);
void* fe_arena_alloc(FeArena* arena, usize size, usize align);

FeArenaState fe_arena_save(FeArena* arena);
void fe_arena_restore(FeArena* arena, FeArenaState save);

// -------------------------------------
// passes/transformations
// -------------------------------------

typedef struct {
    bool warning;
    enum : u8 { // something's wrong with...
        FE_VERIFY_INST,    // an instruction
        FE_VERIFY_BLOCK,   // a block
        FE_VERIFY_FUNC,    // a function
        FE_VERIFY_FUNCSIG, // a function signature
        FE_VERIFY_SYMBOL,  // a symbol
        FE_VERIFY_SECTION, // a section
    } kind;
    char* message;
    union {
        FeInst*    inst;
        FeBlock*   block;
        FeFunc*    func;
        FeFuncSig* funcsig;
        FeSymbol*  symbol;
        FeSection* section;
    };
} FeVerifyReport;

typedef struct {
    FeVerifyReport* reports;
    u32 len;
    u32 cap;
} FeVerifyReportList;

FeVerifyReportList fe_verify_module(FeModule* m);

void fe_opt_local(FeFunc* f);
void fe_opt_tdce(FeFunc* f);
void fe_opt_compact_ids(FeFunc* f);

void fe_opt_post_regalloc(FeFunc* f);

// -------------------------------------
// IO utilities
// -------------------------------------

// like stringbuilder but epic
typedef struct FeDataBuffer {
    u8* at;
    usize len;
    usize cap;
} FeDataBuffer;

void fe_db_init(FeDataBuffer* buf, usize cap);
void fe_db_clone(FeDataBuffer* dst, FeDataBuffer* src);
void fe_db_destroy(FeDataBuffer* buf) ;

char* fe_db_clone_to_cstring(FeDataBuffer* buf);

// reserve bytes
void fe_db_reserve(FeDataBuffer* buf, usize more);

// reserve until buf->cap == new_cap
// does nothing when buf->cap > new_cap
void fe_db_reserve_until(FeDataBuffer* buf, usize new_cap);

// append data to end of buffer
usize fe_db_write(FeDataBuffer* buf, const void* ptr, usize len);
usize fe_db_writecstr(FeDataBuffer* buf, const char* s);
usize fe_db_write8(FeDataBuffer* buf, u8 data);
usize fe_db_write16(FeDataBuffer* buf, u16 data);
usize fe_db_write32(FeDataBuffer* buf, u32 data);
usize fe_db_write64(FeDataBuffer* buf, u64 data);
usize fe_db_writef(FeDataBuffer* buf, const char* fmt, ...);

void fe_emit_ir_func(FeDataBuffer* db, FeFunc* f, bool fancy);
void fe__emit_ir_stack_label(FeDataBuffer* db, FeStackItem* item);
void fe__emit_ir_block_label(FeDataBuffer* db, FeFunc* f, FeBlock* ref);
void fe__emit_ir_ref(FeDataBuffer* db, FeFunc* f, FeInst* ref);

// crash at runtime with a stack trace (if available)
[[noreturn]] void fe_runtime_crash(const char* error, ...);

// initialize signal handler that calls FE_CRASH
// in the event of a bad signal
void fe_init_signal_handler();

// -------------------------------------
// code generation
// -------------------------------------

// vreg hasnt been asigned a real reg yet
#define FE_VREG_REAL_UNASSIGNED UINT16_MAX

#define FE_VREG_NONE UINT32_MAX

typedef struct FeBlockLiveness {
    FeBlock* block;

    u16 in_len;
    u16 in_cap;
    
    u16 out_len;
    u16 out_cap;

    FeVReg* in;
    FeVReg* out;
} FeBlockLiveness;

typedef struct FeVirtualReg {
    u8 class;
    u16 real;
    bool is_phi_input;
    // FeVReg hint;

    FeInst* def;
    FeBlock* def_block;
} FeVirtualReg;

typedef struct FeVRegBuffer {
    FeVirtualReg* at;
    u32 len;
    u32 cap;
} FeVRegBuffer;

typedef enum : u8 {
    FE_REG_CALL_CLOBBERED, // aka "caller-saved"
    FE_REG_CALL_PRESERVED, // aka "callee-saved"
    FE_REG_UNUSABLE,
} FeRegStatus;

typedef u8 FeRegClass;

typedef struct FeTarget {
    FeArch arch;
    FeSystem system;

    FeTy ptr_ty;

    FeInst** (*list_inputs)(FeInst* inst, usize* len_out);
    FeBlock** (*list_targets)(FeInst* term, usize* len_out);

    FeInstChain (*isel)(FeFunc* f, FeBlock* block, FeInst* inst);
    void (*post_regalloc_reduce)(FeFunc* f);
    void (*pre_regalloc_opt)(FeFunc* f);
    void (*final_touchups)(FeFunc* f);

    FeRegClass (*choose_regclass)(FeInstKind kind, FeTy ty);
    const char* (*reg_name)(u8 regclass, u16 real);
    FeRegStatus (*reg_status)(u8 cconv, u8 regclass, u16 real);
    
    u8 num_regclasses;
    const u16* regclass_lens;

    u64 stack_pointer_align;

    void (*ir_print_inst)(FeDataBuffer* db, FeFunc* f, FeInst* inst);

    void (*print_text)(FeDataBuffer* db, FeModule* m);
} FeTarget;

const FeTarget* fe_make_target(FeArch arch, FeSystem system);

void fe_regalloc_basic(FeFunc* f);

void fe_vrbuf_init(FeVRegBuffer* buf, usize cap);
void fe_vrbuf_clear(FeVRegBuffer* buf);
void fe_vrbuf_destroy(FeVRegBuffer* buf);

FeVReg fe_vreg_new(FeVRegBuffer* buf, FeInst* def, FeBlock* def_block, u8 class);
FeVirtualReg* fe_vreg(FeVRegBuffer* buf, FeVReg vr);

void fe_codegen(FeFunc* f);

void fe_cg_print_text(FeDataBuffer* db, FeModule* mod);

#ifdef __cplusplus
}
#endif

#endif
