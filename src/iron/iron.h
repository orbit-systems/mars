#pragma once

#define DONT_USE_MARS_ALLOC
#include "common/orbit.h"
#include "common/alloc.h"
#include "common/arena.h"

#define FE_VERSION_MAJOR 0
#define FE_VERSION_MINOR 1
#define FE_VERSION_PATCH 0

typedef struct FeModule FeModule;
typedef struct FePass   FePass;

typedef u32 FeType;
typedef struct FeInst  FeInst;
typedef        FeInst* FeInstPTR;

da_typedef(FeInstPTR);

typedef struct FeFunction     FeFunction;
typedef struct FeFunctionItem FeFunctionItem;
typedef struct FeData         FeData;
typedef struct FeSymbol       FeSymbol;
typedef struct FeBasicBlock   FeBasicBlock;

typedef struct FePass {
    char* name;
    union {
        void* raw_callback;
        void (*callback)(FeModule*);
    };

    // make sure CFG info is up-to-date before this pass runs
    bool requires_cfg;
    // mark the CFG information as out-of-date after this pass runs
    bool modifies_cfg;
} FePass;

void fe_sched_pass(FeModule* m, FePass* p);
void fe_sched_pass_at(FeModule* m, FePass* p, int index);
void fe_run_next_pass(FeModule* m, bool printout);
void fe_run_all_passes(FeModule* m, bool printout);

enum {
    FE_TYPE_VOID,

    FE_TYPE_BOOL,

    FE_TYPE_PTR,

    FE_TYPE_I8,
    FE_TYPE_I16,
    FE_TYPE_I32,
    FE_TYPE_I64,

    FE_TYPE_F16,
    FE_TYPE_F32,
    FE_TYPE_F64,

    _FE_TYPE_SIMPLE_END,

    FE_TYPE_RECORD,
    FE_TYPE_ARRAY,
};

typedef struct FeAggregateType {
    u8 kind;

    union {
    
        struct {
            u64 len;
            FeType fields[]; // flexible array member
        } record;

        struct {
            u64 len;
            FeType sub;
        } array;
    };
} FeAggregateType;

FeType fe_type_array(FeModule* m, FeType subtype, u64 len);
FeType fe_type_record(FeModule* m, u64 len);

bool fe_type_is_scalar(FeType t);
bool fe_type_has_equivalence(FeType t);
bool fe_type_has_ordering(FeType t);
bool fe_type_is_integer(FeType t);
bool fe_type_is_float(FeType t);

// returns null for non-aggregate types
FeAggregateType* fe_type_get_structure(FeModule* m, FeType t);

enum {
    FE_BIND_EXPORT,
    FE_BIND_EXPORT_WEAK,
    FE_BIND_LOCAL,
    FE_BIND_IMPORT,
};

typedef struct FeSymbol {
    string name;
    union {
        // void* ref;
        FeFunction* function;
        FeData* data;
    };
    bool is_function;
    u8 binding;
} FeSymbol;

enum {
    FE_DATA_NONE = 0,
    FE_DATA_BYTES,
    FE_DATA_SYMREF,

    _FE_DATA_NUMERIC_BEGIN,
    FE_DATA_D8,
    FE_DATA_D16,
    FE_DATA_D32,
    FE_DATA_D64,
    _FE_DATA_NUMERIC_END,
};

typedef struct FeData {
    FeSymbol* sym;

    union {
        struct {
            u8* data;
            u32 len;
            bool zeroed;
        } bytes;
        FeSymbol* symref;

        u8  d8;
        u16 d16;
        u32 d32;
        u64 d64;
    };

    u8 kind;
    bool read_only;
} FeData;

#define FE_FN_ALLOCA_BLOCK_SIZE 0x4000

typedef struct FeStackObject {
    FeType t;
} FeStackObject;

typedef struct FeFunction {
    FeModule* mod;
    FeSymbol* sym;

    struct {
        FeBasicBlock** at;
        u32 len;
        u32 cap;
    } blocks;

    struct {
        FeStackObject** at;
        u32 len;
        u32 cap;
    } stack;

    struct {
        FeFunctionItem** at;
        u16 len;
        u16 cap;
    } params;

    struct {
        FeFunctionItem** at;
        u16 len;
        u16 cap;
    } returns;

    Arena alloca;
} FeFunction;

typedef struct FeFunctionItem {

    FeType type;

    // if its an aggregate, this is exposed as a pointer marked "by_val" so that
    // calling conventions work without needing target-specific information.
    FeType by_value;
} FeFunctionItem;

typedef struct FeCFGNode FeCFGNode;
typedef struct FeCFGNode {
    FeBasicBlock* bb; // if null, this CFG node is invalid
    FeCFGNode** incoming;
    FeCFGNode** outgoing;

    FeCFGNode* immediate_dominator;
    
    // set of all nodes this node strictly dominates
    FeCFGNode** dominates;
    
    // dominance frontier for this node
    FeCFGNode** domfront;

    u16 out_len;
    u16 in_len;
    u32 dominates_len;
    u32 domfront_len;

    u32 pre_order_index; // starts at 1 for the entry block

    u64 flags; // misc flags, for storing any kind of extra data
} FeCFGNode;

typedef struct FeBasicBlock {
    FeFunction* function;

    FeInst* start;
    FeInst* end;
    string name;

    FeCFGNode* cfg_node;

    u64 flags; // for misc use
} FeBasicBlock;

enum {
    FE_INST_INVALID,
    
    // that signals the ending of a basic block.
    // contains a backlink to the basic block itself.
    // basic block (.end) will ONLY point to this IF
    // there are NO other instructions in the block.
    FE_INST_BOOKEND,

    _FE_INST_NO_SIDE_EFFECTS_BEGIN,

    // FeInstBinop
    _FE_BINOP_BEGIN,

        FE_INST_ADD,
        FE_INST_SUB,
        FE_INST_IMUL,
        FE_INST_UMUL,
        FE_INST_IDIV,
        FE_INST_UDIV,
        FE_INST_IMOD,
        FE_INST_UMOD,

        FE_INST_FADD,
        FE_INST_FSUB,
        FE_INST_FMUL,
        FE_INST_FDIV,
        FE_INST_FMOD,

        _FE_INST_CMP_START,

            // FeInstBinop
            FE_INST_ULT, // <
            FE_INST_UGT, // >
            FE_INST_ULE, // <=
            FE_INST_UGE, // >=
            FE_INST_ILT, // <
            FE_INST_IGT, // >
            FE_INST_ILE, // <=
            FE_INST_IGE, // >=
            FE_INST_EQ, // ==
            FE_INST_NE, // !=

        _FE_INST_CMP_END,

        // FeBinop
        FE_INST_AND,
        FE_INST_OR,
        FE_INST_XOR,
        FE_INST_SHL,
        FE_INST_ASR,
        FE_INST_LSR,

    _FE_BINOP_END,

    // FeInstUnop
    FE_INST_NOT,
    FE_INST_NEG,
    FE_INST_CAST,
    FE_INST_BITCAST,

    // FeInstStackAddr
    FE_INST_STACK_ADDR,

    // FeInstFieldPtr
    FE_INST_FIELDPTR,

    // FeInstIndexPtr
    FE_INST_INDEXPTR,

    // FeInstConst
    FE_INST_CONST,
    // FeLoadSymbol
    FE_INST_LOAD_SYMBOL,

    // FeInstMov
    FE_INST_MOV,
    // FeInstPhi
    FE_INST_PHI,

    // FeInstParamVal
    FE_INST_PARAMVAL,

    _FE_INST_NO_SIDE_EFFECTS_END,

    // FeInstLoad
    FE_INST_LOAD,
    FE_INST_VOL_LOAD,
    // FeInstStackLoad
    FE_INST_STACK_LOAD,

    // FeInstStore
    FE_INST_STORE,
    FE_INST_VOL_STORE,
    FE_INST_STACK_STORE,

    // FeInstBranch
    FE_INST_BRANCH,
    // FeInstJump
    FE_INST_JUMP,

    // FeInstReturnVal
    FE_INST_RETURNVAL,
    // FeInstReturn
    FE_INST_RETURN,

    // FeInstProvide
    FE_INST_PROVIDE,
    // FeInstRetrieve
    FE_INST_RETRIEVE,
    // FeInstCall
    FE_INST_CALL,
    // FeInstIndirectCall
    FE_INST_INDIRECT_CALL,
    // FeInstAsm
    FE_INST_ASM,

    _FE_INST_MAX,
};

// basic IR structure
typedef struct FeInst {
    u8 kind;
    u16 use_count;
    FeType type;
    FeInst* next;
    FeInst* prev;    
} FeInst;

typedef struct FeInstBookend {
    FeInst base;
    FeBasicBlock* bb;
} FeInstBookend;

typedef struct FeInstBinop {
    FeInst base;

    FeInst* lhs;
    FeInst* rhs;
} FeInstBinop;

typedef struct FeInstUnop {
    FeInst base;

    FeInst* source;
} FeInstUnop;


typedef struct FeInstStackAddr {
    FeInst base;

    FeStackObject* object;
} FeInstStackAddr;

// used for struct accesses
typedef struct FeInstFieldPtr {
    FeInst base;

    FeInst* source;
    u32 index;
} FeInstFieldPtr;

// used for array accesses
typedef struct FeInstIndexPtr {
    FeInst base;

    FeInst* source;
    FeInst* index;
} FeInstIndexPtr;

/*
    load/store alignment

    the .align_offset field on FeInstLoad and FeInstStore indicates how misaligned
    the pointer is from the type's natural alignment. if align_offset is greater or 
    equal to the types natural alignment (a good catch-all is 255), the pointer's 
    alignment is treated as unknown.

    if align_offset is known but not zero, a codegen backend can optimize to a
    specific misalignment case. for example: a load.i64 with align_offset=4 can be 
    translated to two 4-byte load instructions. a load.i64 with unknown (>=8) offset
    might be translated into several i8 loads.
*/

#define FE_MEMOP_UNKNOWN_ALIGNMENT 255

typedef struct FeInstLoad {
    FeInst base;

    FeInst* location;

    u8 align_offset;
} FeInstLoad;

typedef struct FeInstStore {
    FeInst base;

    FeInst* location;
    FeInst* value;

    u8 align_offset;
} FeInstStore;

typedef struct FeInstStackLoad {
    FeInst base;

    FeStackObject* location;
} FeInstStackLoad;

typedef struct FeInstStackStore {
    FeInst base;

    FeStackObject* location;
    FeInst* value;

} FeInstStackStore;

typedef struct FeInstConst {
    FeInst base;

    union {
        bool bool : 1;
        
        i8  i8;
        i16 i16;
        i32 i32;
        i64 i64;

        f16 f16;
        f32 f32;
        f64 f64;
    };
} FeInstConst;

typedef struct FeLoadSymbol {
    FeInst base;
    
    FeSymbol* sym;
} FeLoadSymbol;

typedef struct FeInstMov {
    FeInst base;

    FeInst* source;
} FeInstMov;

typedef struct FeInstPhi {
    FeInst base;

    FeInst** sources;
    FeBasicBlock** source_BBs;
    u16 len;
    u16 cap;
} FeInstPhi;

typedef struct FeInstJump {
    FeInst base;

    FeBasicBlock* dest;
} FeInstJump;

typedef struct FeInstBranch {
    FeInst base;

    FeInst* cond;

    FeBasicBlock* if_true;
    FeBasicBlock* if_false;
} FeInstBranch;

// get value from register parameter OR the pointer to a stack parameter.
// if register, lifetime of the register starts from the start of the entry 
// basic block and continues to this node.
// MUST BE THE FIRST INSTRUCTION IN THE ENTRY BLOCK OR IN A SEQUENCE OF 
// OTHER FeInstParamVal INSTRUCTIONS
typedef struct FeInstParamVal {
    FeInst base;

    u32 index;
} FeInstParamVal;

// set register return val
typedef struct FeInstReturnVal {
    FeInst base;
    FeInst* source;

    u32 index;
} FeInstReturnVal;

typedef struct FeInstReturn {
    FeInst base;
} FeInstReturn;

typedef struct FeInstProvide {
    FeInst base;

    FeInst* source;
    u16 index;
} FeInstProvide;

typedef struct FeInstRetrieve {
    FeInst base;

    u16 index;
} FeInstProvide;

typedef struct FeInstCall {
    FeInst base;

    FeFunction* source;
    // derives calling convention from source
} FeInstCall;

typedef struct FeInstIndirectCall {
    FeInst base;
    FeInst* source;

    u16 callconv;
} FeInstIndirectCall;

typedef struct FeInstAsm {
    FeInst base;

    // assembly text
    string text;
} FeInstAsm;

enum {
    // C calling convention on the target platform
    FE_CALLCONV_CDECL,
    // force stdcall as defined by windows
    FE_CALLCONV_STDCALL,
    // force system v as defined on linux, etc.
    FE_CALLCONV_SYSV

    // parameters are not defined to be passed in any specific way, the backend can choose
    // can only appear on functions with FE_BIND_LOCAL symbolic binding
    // FE_CALLCONV_OPT,
};

#define for_fe_inst(inst, basic_block) for(FeInst* inst = (basic_block).start; inst->kind != FE_INST_BOOKEND; inst = inst->next)
#define for_fe_inst_from(inst, start, basic_block) for(FeInst* inst = start; inst->kind != FE_INST_BOOKEND; inst = inst->next)

extern const size_t fe_inst_sizes[];

FeModule*     fe_new_module(string name);
FeFunction*   fe_new_function(FeModule* mod, FeSymbol* sym);
FeBasicBlock* fe_new_basic_block(FeFunction* fn, string name);
FeData*       fe_new_data(FeModule* mod, FeSymbol* sym, bool read_only);
FeSymbol*     fe_new_symbol(FeModule* mod, string name, u8 binding);
FeSymbol*     fe_find_symbol(FeModule* mod, string name);
FeSymbol*     fe_find_or_new_symbol(FeModule* mod, string name, u8 binding);

void fe_destroy_module(FeModule* m);
void fe_destroy_function(FeFunction* f);
void fe_destroy_basic_block(FeBasicBlock* bb);

FeStackObject* fe_new_stackobject(FeFunction* f, FeType t);
void fe_init_func_params(FeFunction* f, u16 count);
void fe_init_func_returns(FeFunction* f, u16 count);
FeFunctionItem* fe_add_func_param(FeFunction* f, FeType t);
FeFunctionItem* fe_add_func_return(FeFunction* f, FeType t);
u32  fe_bb_index(FeFunction* fn, FeBasicBlock* bb);
void fe_set_data_bytes(FeData* data, u8* bytes, u32 data_len, bool zeroed);
void fe_set_data_symref(FeData* data, FeSymbol* symref);

FeInst* fe_append(FeBasicBlock* bb, FeInst* ir);
FeInst* fe_insert_inst_before(FeInst* new, FeInst* ref);
FeInst* fe_insert_inst_after(FeInst* new, FeInst* ref);
FeInst* fe_remove(FeInst* inst);
FeInst* fe_move_before(FeInst* inst, FeInst* ref);
FeInst* fe_move_after(FeInst* inst, FeInst* ref);
void    fe_rewrite_uses(FeFunction* f, FeInst* source, FeInst* dest);
void    fe_add_uses_to_worklist(FeFunction* f, FeInst* source, da(FeInstPTR)* worklist);
bool    fe_inst_is_terminator(FeInst* inst);

FeInst* fe_inst(FeFunction* f, u8 type);
FeInst* fe_inst_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs);
FeInst* fe_inst_unop(FeFunction* f, u8 type, FeInst* source);
FeInst* fe_inst_stackaddr(FeFunction* f, FeStackObject* obj);
FeInst* fe_inst_getfieldptr(FeFunction* f, u32 index, FeInst* source);
FeInst* fe_inst_getindexptr(FeFunction* f, FeInst* index, FeInst* source);
FeInst* fe_inst_load(FeFunction* f, FeInst* ptr, FeType as, bool is_vol);
FeInst* fe_inst_store(FeFunction* f, FeInst* ptr, FeInst* value, bool is_vol);
FeInst* fe_inst_stack_load(FeFunction* f, FeStackObject* location);
FeInst* fe_inst_stack_store(FeFunction* f, FeStackObject* location, FeInst* value);
FeInst* fe_inst_const(FeFunction* f, FeType type);
FeInst* fe_inst_load_symbol(FeFunction* f, FeType type, FeSymbol* symbol);
FeInst* fe_inst_mov(FeFunction* f, FeInst* source);
FeInst* fe_inst_phi(FeFunction* f, u32 count, FeType type);
FeInst* fe_inst_jump(FeFunction* f, FeBasicBlock* dest);
FeInst* fe_inst_branch(FeFunction* f, FeInst* cond, FeBasicBlock* if_true, FeBasicBlock* if_false);
FeInst* fe_inst_paramval(FeFunction* f, u32 param);
FeInst* fe_inst_returnval(FeFunction* f, u32 param, FeInst* source);
FeInst* fe_inst_return(FeFunction* f);

void fe_add_phi_source(FeFunction* f, FeInstPhi* phi, FeInst* source, FeBasicBlock* source_block);

string fe_emit_ir(FeModule* m);
FeModule* fe_read_module(string text);

string fe_emit_c(FeModule* m);

typedef struct FeMessage {
    u8 severity;
    u8 kind;
    
    char* function_of_origin;
    char* message;

    union {
        
    };

} FeMessage;

typedef struct FeMessageQueue {
    FeMessage* at;
    size_t len;
    size_t cap;
} FeMessageQueue;

enum {
    FE_MSG_KIND_NONE,
    FE_MSG_KIND_IR,
    FE_MSG_KIND_MACH_IR,
};

enum {
    FE_MSG_SEVERITY_FATAL,
    FE_MSG_SEVERITY_ERROR,
    FE_MSG_SEVERITY_WARNING,
    FE_MSG_SEVERITY_LOG,
};

// if the message is fatal, the error is immediately printed
void fe_push_message(FeModule* m, FeMessage msg);
FeMessage fe_pop_message(FeModule* m);
void fe_clear_message_buffer(FeModule* m);
void fe_print_message(FeMessage msg);

enum {
    _FE_ARCH_BEGIN,

    FE_ARCH_X86_64,   // x86-64
    FE_ARCH_APHELION, // aphelion 
    FE_ARCH_ARM64,    // arm64
    FE_ARCH_XR17032,  // xr/17032
    FE_ARCH_FOX32,    // fox32

    _FE_ARCH_END,
};

enum {
    _FE_SYSTEM_BEGIN,

    FE_SYSTEM_NONE,      // freestanding
    
    _FE_SYSTEM_END,
};

typedef struct FeModule {
    string name;

    FeFunction** functions;
    FeData** datas;

    u32 functions_len;
    u32 datas_len;

    struct {
        FeSymbol** at;
        size_t len;
        size_t cap;

        Arena alloca;
    } symtab; // symbol table

    struct {
        FeAggregateType** at;
        size_t len;
        size_t cap;

        Arena alloca;
    } typegraph;

    struct {
        FePass** at;
        size_t len;
        size_t cap;

        bool cfg_up_to_date;
    } pass_queue;

    struct {
        u16 arch;
        u16 system;
        void* arch_config;
        void* system_config;
    } target;

    FeMessageQueue messages;
} FeModule;

FeType fe_arch_type_of_native_int(u16 arch);
FeType fe_arch_type_of_native_float(u16 arch);
bool   fe_arch_type_is_native(u16 arch, FeType t);

typedef struct FeAllocator {
    void* (*malloc)(size_t);
    void* (*realloc)(void*, size_t);
    void  (*free)(void*);
} FeAllocator;

void  fe_set_allocator(FeAllocator alloc);
void* fe_malloc(size_t size);
void* fe_realloc(void* ptr, size_t size);
void  fe_free(void* ptr);

#include "iron/passes/passes.h"
#include "iron/codegen/mach.h"