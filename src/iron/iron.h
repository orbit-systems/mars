#pragma once

#define DONT_USE_MARS_ALLOC
#include "common/orbit.h"
#include "common/alloc.h"
#include "common/arena.h"

#define FE_VERSION_MAJOR 0
#define FE_VERSION_MINOR 1
#define FE_VERSION_PATCH 0

typedef struct FeModule FeModule;
typedef struct FeFunction FeFunction;
typedef struct FeFunctionItem FeFunctionItem;
typedef struct FeData FeData;
typedef struct FeSymbol FeSymbol;
typedef struct FeBasicBlock FeBasicBlock;

typedef u32 FeType;
typedef struct FeIr FeIr;
typedef FeIr* FeIrPTR;

da_typedef(FeIrPTR);

typedef struct FePass {
    char* name;
    void (*module)(FeModule*);        // run on the entire module.
    void (*function)(FeFunction* fn); // run on a function.
} FePass;

enum {
    FE_SCHED_MODULE,
    FE_SCHED_FUNCTION,
};

typedef struct FeSchedPass {
    FePass* pass;
    union {
        FeFunction* fn;
    } bind;
    u8 sched_kind;
} FeSchedPass;

void fe_sched_func_pass(FeModule* m, FePass* p, FeFunction* fn);
void fe_sched_module_pass(FeModule* m, FePass* p);
void fe_sched_func_pass_at(FeModule* m, FePass* p, FeFunction* fn, u64 index);
void fe_sched_module_pass_at(FeModule* m, FePass* p, u64 index);
void fe_run_next_pass(FeModule* m);
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

        u8 d8;
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

    u8 cconv;

    bool cfg_up_to_date;

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

    Arena cfg;
    Arena alloca;
} FeFunction;

typedef struct FeFunctionItem {
    FeType type;
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

    FeIr* start;
    FeIr* end;
    string name;

    FeCFGNode* cfg_node;

    u64 flags; // for misc use
} FeBasicBlock;

enum {
    FE_IR_INVALID,

    // that signals the beginning/ending of a basic block.
    // contains a backlink to the basic block itself.
    // basic block start/end will ONLY point to this if
    // there are no other instructions in the block.
    FE_IR_BOOKEND,

    _FE_IR_NO_SIDE_EFFECTS_BEGIN,

    // FeIrBinop
    _FE_IR_BINOP_BEGIN,

    FE_IR_ADD,
    FE_IR_SUB,
    FE_IR_IMUL,
    FE_IR_UMUL,
    FE_IR_IDIV,
    FE_IR_UDIV,
    FE_IR_IMOD,
    FE_IR_UMOD,

    FE_IR_FADD,
    FE_IR_FSUB,
    FE_IR_FMUL,
    FE_IR_FDIV,
    FE_IR_FMOD,

    _FE_IR_CMP_START,

    // FeIrBinop
    FE_IR_ULT, // <
    FE_IR_UGT, // >
    FE_IR_ULE, // <=
    FE_IR_UGE, // >=
    FE_IR_ILT, // <
    FE_IR_IGT, // >
    FE_IR_ILE, // <=
    FE_IR_IGE, // >=
    FE_IR_EQ,  // ==
    FE_IR_NE,  // !=

    _FE_IR_CMP_END,

    // FeBinop
    FE_IR_AND,
    FE_IR_OR,
    FE_IR_XOR,
    FE_IR_SHL,
    FE_IR_ASR,
    FE_IR_LSR,

    _FE_BINOP_END,

    // FeIrUnop
    FE_IR_NOT,
    FE_IR_NEG,

    // FeIrUnop
    FE_IR_BITCAST,

    // FeIrUnop
    FE_IR_TRUNC,
    FE_IR_SIGNEXT,
    FE_IR_ZEROEXT,

    // FeIrStackAddr
    FE_IR_STACK_ADDR,

    // FeIrFieldPtr
    FE_IR_FIELD_PTR,
    // FeIrIndexPtr
    FE_IR_INDEX_PTR,

    // FeIrGetField
    FE_IR_GET_FIELD,
    // FeIrSetField
    FE_IR_SET_FIELD,

    // FeIrGetIndex
    FE_IR_GET_INDEX,
    // FeIrSetIndex
    FE_IR_SET_INDEX,

    // FeIrConst
    FE_IR_CONST,
    // FeIrLoadSymbol
    FE_IR_LOAD_SYMBOL,

    // FeIrMov
    FE_IR_MOV,
    // FeIrPhi
    FE_IR_PHI,

    // FeIrParam
    FE_IR_PARAM,

    _FE_IR_NO_SIDE_EFFECTS_END,

    // FeIrLoad
    FE_IR_LOAD,
    FE_IR_VOL_LOAD,
    // FeIrStackLoad
    FE_IR_STACK_LOAD,

    // FeIrStore
    FE_IR_STORE,
    FE_IR_VOL_STORE,
    FE_IR_STACK_STORE,

    // FeIrBranch
    FE_IR_BRANCH,
    // FeIrJump
    FE_IR_JUMP,

    // FeIrReturn
    FE_IR_RETURN,

    // FeIrRetrieve
    FE_IR_RETRIEVE,
    // FeIrCall
    FE_IR_CALL,
    // FeIrPtrCall
    FE_IR_PTR_CALL,
    // FeIrAsm
    FE_IR_ASM,

    _FE_IR_MAX,

    _FE_IR_ARCH_SPECIFIC_START,
};

// basic IR structure
typedef struct FeIr {
    u16 kind;
    u16 flags;
    FeType type;
    FeIr* next;
    FeIr* prev;
} FeIr;

typedef struct FeIrArchInst {
    FeIr base;
    FeIr* sources[]; // variable length field
} FeIrArchInst;

// information about an arch inst
typedef struct FeIrArchInstInfo {
    const char* name;
    u16 sources_len;
} FeIrArchInstInfo;

typedef struct FeIrBookend {
    FeIr base;
    FeBasicBlock* bb;
} FeIrBookend;

typedef struct FeIrBinop {
    FeIr base;

    FeIr* lhs;
    FeIr* rhs;
} FeIrBinop;

typedef struct FeIrUnop {
    FeIr base;

    FeIr* source;
} FeIrUnop;

typedef struct FeIrStackAddr {
    FeIr base;

    FeStackObject* object;
} FeIrStackAddr;

// used for struct accesses
typedef struct FeIrFieldPtr {
    FeIr base;

    FeIr* source;
    u32 index;
} FeIrFieldPtr;

typedef struct FeIrSetField {
    FeIr base;

    FeIr* record;
    FeIr* source;
    u32 index;
} FeIrSetField;

typedef struct FeIrGetField {
    FeIr base;

    FeIr* record;
    u32 index;
} FeIrGetField;

// used for array accesses
typedef struct FeIrIndexPtr {
    FeIr base;

    FeIr* source;
    FeIr* index;
} FeIrIndexPtr;

typedef struct FeIrSetIndex {
    FeIr base;

    FeIr* record;
    u32 index;
} FeIrSetIndex;

typedef struct FeIrGetIndex {
    FeIr base;

    FeIr* array;
    u32 index;
} FeIrGetIndex;

/*
    load/store alignment

    the .align_offset field on FeIrLoad and FeIrStore indicates how misaligned
    the pointer is from the type's natural alignment. if align_offset is greater or
    equal to the types natural alignment (a good catch-all is 255), the pointer's
    alignment is treated as unknown.

    if align_offset is known but not zero, a codegen backend can optimize to a
    specific misalignment case. for example: a load.i64 with align_offset=4 can be
    translated to two 4-byte load instructions. a load.i64 with unknown (>=8) offset
    might be translated into several i8 loads.
*/

#define FE_MEMOP_UNKNOWN_ALIGNMENT 255

typedef struct FeIrLoad {
    FeIr base;

    FeIr* location;

    u8 align_offset;
} FeIrLoad;

typedef struct FeIrStore {
    FeIr base;

    FeIr* location;
    FeIr* value;

    u8 align_offset;
} FeIrStore;

typedef struct FeIrStackLoad {
    FeIr base;

    FeStackObject* location;
} FeIrStackLoad;

typedef struct FeIrStackStore {
    FeIr base;

    FeStackObject* location;
    FeIr* value;

} FeIrStackStore;

typedef struct FeIrConst {
    FeIr base;

    union {
        bool bool : 1;

        i8 i8;
        i16 i16;
        i32 i32;
        i64 i64;

        f16 f16;
        f32 f32;
        f64 f64;
    };
} FeIrConst;

typedef struct FeIrLoadSymbol {
    FeIr base;

    FeSymbol* sym;
} FeIrLoadSymbol;

typedef struct FeIrMov {
    FeIr base;
    FeIr* source;
} FeIrMov;

typedef struct FeIrPhi {
    FeIr base;

    FeIr** sources;
    FeBasicBlock** source_BBs;
    u16 len;
    u16 cap;
} FeIrPhi;

typedef struct FeIrJump {
    FeIr base;

    FeBasicBlock* dest;
} FeIrJump;

typedef struct FeIrBranch {
    FeIr base;

    FeIr* cond;

    FeBasicBlock* if_true;
    FeBasicBlock* if_false;
} FeIrBranch;

typedef struct FeIrParam {
    FeIr base;

    u32 index;
} FeIrParam;

typedef struct FeIrReturn {
    FeIr base;

    FeIr** sources;
    u16 len; // sync'd with function return len

} FeIrReturn;

typedef struct FeIrCall {
    FeIr base;

    FeFunction* source;
    // derives calling convention from source

    FeIr** params;
    u16 params_len;
} FeIrCall;

typedef struct FeIrPtrCall {
    FeIr base;
    FeIr* source;

    u16 callconv;
} FeIrPtrCall;

typedef struct FeIrRetrieve {
    FeIr base;

    FeIr* call;

    u16 index;
} FeIrRetrieve;

typedef struct FeIrAsm {
    FeIr base;

    // KINDA TODO LMAO

    FeIr** params;
    u16 params_len;

    // assembly text
    string text;
} FeIrAsm;

enum {

    // C calling convention on the target platform (sys-v/stdcall)
    FE_CCONV_CDECL,

    // force windows stdcall
    FE_CCONV_STDCALL,
    // force system v
    FE_CCONV_SYSV,

    // none of the above calling conventions support multi-returns natively.
    // a FeReport of FE_REP_SEVERITY_ERROR severity will be generated at codegen.

    // mars calling convention, native multi-return support
    FE_CCONV_MARS,

    // jackal calling convention, native multi-return support
    FE_CCONV_JACKAL,

    // parameters are not defined to be passed in any specific way, the backend can choose
    // can only appear on functions with FE_BIND_LOCAL symbolic binding
    // FE_CCONV_OPT,
};

#define for_fe_ir(inst, basic_block) for (FeIr* inst = (basic_block).start; inst->kind != FE_IR_BOOKEND; inst = inst->next)
#define for_fe_ir_from(inst, start, basic_block) for (FeIr* inst = start; inst->kind != FE_IR_BOOKEND; inst = inst->next)

extern const size_t fe_inst_sizes[];

FeModule* fe_new_module(string name);
FeFunction* fe_new_function(FeModule* mod, FeSymbol* sym, u8 cconv);
FeBasicBlock* fe_new_basic_block(FeFunction* fn, string name);
FeData* fe_new_data(FeModule* mod, FeSymbol* sym, bool read_only);
FeSymbol* fe_new_symbol(FeModule* mod, string name, u8 binding);
FeSymbol* fe_find_symbol(FeModule* mod, string name);
FeSymbol* fe_find_or_new_symbol(FeModule* mod, string name, u8 binding);

void fe_destroy_module(FeModule* m);
void fe_destroy_function(FeFunction* f);
void fe_destroy_basic_block(FeBasicBlock* bb);

FeStackObject* fe_new_stackobject(FeFunction* f, FeType t);
void fe_init_func_params(FeFunction* f, u16 count);
void fe_init_func_returns(FeFunction* f, u16 count);
FeFunctionItem* fe_add_func_param(FeFunction* f, FeType t);
FeFunctionItem* fe_add_func_return(FeFunction* f, FeType t);
u32 fe_bb_index(FeFunction* fn, FeBasicBlock* bb);
void fe_set_data_bytes(FeData* data, u8* bytes, u32 data_len, bool zeroed);
void fe_set_data_symref(FeData* data, FeSymbol* symref);

FeIr* fe_append_ir(FeBasicBlock* bb, FeIr* ir);
FeIr* fe_insert_ir_before(FeIr* new, FeIr* ref);
FeIr* fe_insert_ir_after(FeIr* new, FeIr* ref);
FeIr* fe_remove_ir(FeIr* inst);
FeIr* fe_move_ir_before(FeIr* inst, FeIr* ref);
FeIr* fe_move_ir_after(FeIr* inst, FeIr* ref);
void fe_rewrite_ir_uses(FeFunction* f, FeIr* source, FeIr* dest);
void fe_add_ir_uses_to_worklist(FeFunction* f, FeIr* source, da(FeIrPTR) * worklist);
bool fe_is_ir_terminator(FeIr* inst);

FeIr* fe_ir(FeFunction* f, u16 type);
FeIr* fe_ir_binop(FeFunction* f, u16 type, FeIr* lhs, FeIr* rhs);
FeIr* fe_ir_unop(FeFunction* f, u16 type, FeIr* source);
FeIr* fe_ir_stackaddr(FeFunction* f, FeStackObject* obj);
FeIr* fe_ir_field_ptr(FeFunction* f, u32 index, FeIr* source);
FeIr* fe_ir_index_ptr(FeFunction* f, FeIr* index, FeIr* source);
FeIr* fe_ir_load(FeFunction* f, FeIr* ptr, FeType as, bool is_vol);
FeIr* fe_ir_store(FeFunction* f, FeIr* ptr, FeIr* value, bool is_vol);
FeIr* fe_ir_stack_load(FeFunction* f, FeStackObject* location);
FeIr* fe_ir_stack_store(FeFunction* f, FeStackObject* location, FeIr* value);
FeIr* fe_ir_const(FeFunction* f, FeType type);
FeIr* fe_ir_load_symbol(FeFunction* f, FeType type, FeSymbol* symbol);
FeIr* fe_ir_mov(FeFunction* f, FeIr* source);
FeIr* fe_ir_phi(FeFunction* f, u32 count, FeType type);
FeIr* fe_ir_jump(FeFunction* f, FeBasicBlock* dest);
FeIr* fe_ir_branch(FeFunction* f, FeIr* cond, FeBasicBlock* if_true, FeBasicBlock* if_false);
FeIr* fe_ir_param(FeFunction* f, u32 param);
FeIr* fe_ir_return(FeFunction* f);

void fe_add_phi_source(FeFunction* f, FeIrPhi* phi, FeIr* source, FeBasicBlock* source_block);

string fe_emit_ir(FeModule* m);
FeModule* fe_read_module(string text);

string fe_emit_c(FeModule* m);

typedef struct FeReport {
    u8 severity;
    u8 kind;

    char* function_of_origin;
    char* message;

    union {
    };

} FeReport;

typedef struct FeReportQueue {
    FeReport* at;
    size_t len;
    size_t cap;
} FeReportQueue;

enum {
    FE_REP_KIND_NONE,
    FE_REP_KIND_IR,
    FE_REP_KIND_MACH_IR,
};

enum {
    FE_REP_SEVERITY_FATAL,
    FE_REP_SEVERITY_ERROR,
    FE_REP_SEVERITY_WARNING,
    FE_REP_SEVERITY_LOG,
};

// if the report is fatal, the error is immediately printed
void fe_push_report(FeModule* m, FeReport msg);
FeReport fe_pop_report(FeModule* m);
void fe_clear_report_buffer(FeModule* m);
void fe_print_report(FeReport msg);

enum {
    _FE_SYSTEM_BEGIN,

    FE_SYSTEM_NONE, // freestanding
    FE_SYSTEM_LINUX,
    FE_SYSTEM_WINDOWS,

    _FE_SYSTEM_END,
};

typedef struct FeMachBuffer FeMachBuffer;
typedef struct FeDataBuffer FeDataBuffer;
typedef struct FeMachInstTemplate FeMachInstTemplate;

typedef struct FeArchRegclass {
    u8 id;
    u8 len; // number of registers in this class
} FeArchRegclass;

typedef struct FeArchInfo {
    const char* name;

    FeType native_int;
    FeType native_float;

    FeMachBuffer (*cg)(FeModule*);
    void (*emit_text)(FeDataBuffer*, FeMachBuffer*);
    void (*emit_obj)(FeDataBuffer*, FeMachBuffer*);
    void (*emit_exe)(FeDataBuffer*, FeMachBuffer*);

    struct {
        const FeMachInstTemplate* at;
        u32 len;
    } inst_templates;

    struct {
        const FeArchRegclass* at;
        u8 len;
    } regclasses;
} FeArchInfo;

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
        FeSchedPass* at;
        size_t len;
        size_t cap;
    } pass_queue;

    struct {
        const FeArchInfo* arch;
        u16 system;
        void* arch_config;
        void* system_config;
    } target;

    FeReportQueue messages;
} FeModule;

typedef struct FeAllocator {
    void* (*malloc)(size_t);
    void* (*realloc)(void*, size_t);
    void (*free)(void*);
} FeAllocator;

void fe_set_allocator(FeAllocator alloc);
void* fe_malloc(size_t size);
void* fe_realloc(void* ptr, size_t size);
void fe_free(void* ptr);

// like stringbuilder but epic
typedef struct FeDataBuffer {
    u8* at;
    size_t len;
    size_t cap;
} FeDataBuffer;

FeDataBuffer fe_db_new(size_t initial_capacity);
FeDataBuffer fe_db_clone(FeDataBuffer* buf);
FeDataBuffer fe_db_new_from_file(FeDataBuffer* buf, string path);

string fe_db_clone_to_string(FeDataBuffer* buf);
char* fe_db_clone_to_cstring(FeDataBuffer* buf);

// reserve bytes
void fe_db_reserve(FeDataBuffer* buf, size_t more);

// reserve until buf->cap == new_cap
// does nothing when buf->cap > new_cap
void fe_db_reserve_until(FeDataBuffer* buf, size_t new_cap);

// append data to end of buffer
size_t fe_db_write_string(FeDataBuffer* buf, string s);
size_t fe_db_write_bytes(FeDataBuffer* buf, void* ptr, size_t len);
size_t fe_db_write_cstring(FeDataBuffer* buf, char* s);
size_t fe_db_write_format(FeDataBuffer* buf, char* fmt, ...);
size_t fe_db_write_8(FeDataBuffer* buf, u8 data);
size_t fe_db_write_16(FeDataBuffer* buf, u16 data);
size_t fe_db_write_32(FeDataBuffer* buf, u32 data);
size_t fe_db_write_64(FeDataBuffer* buf, u64 data);

// insert data at a certain point, growing the buffer
size_t fe_db_insert_string(FeDataBuffer* buf, size_t at, string s);
size_t fe_db_insert_bytes(FeDataBuffer* buf, size_t at, void* ptr, size_t len);
size_t fe_db_insert_cstring(FeDataBuffer* buf, size_t at, char* s);
size_t fe_db_insert_8(FeDataBuffer* buf, size_t at, u8 data);
size_t fe_db_insert_16(FeDataBuffer* buf, size_t at, u16 data);
size_t fe_db_insert_32(FeDataBuffer* buf, size_t at, u32 data);
size_t fe_db_insert_64(FeDataBuffer* buf, size_t at, u64 data);

// overwrite data at a certain point, growing the buffer if needed
size_t fe_db_overwrite_string(FeDataBuffer* buf, size_t at, string s);
size_t fe_db_overwrite_bytes(FeDataBuffer* buf, size_t at, void* ptr, size_t len);
size_t fe_db_overwrite_cstring(FeDataBuffer* buf, size_t at, char* s);
size_t fe_db_overwrite_8(FeDataBuffer* buf, size_t at, u8 data);
size_t fe_db_overwrite_16(FeDataBuffer* buf, size_t at, u16 data);
size_t fe_db_overwrite_32(FeDataBuffer* buf, size_t at, u32 data);
size_t fe_db_overwrite_64(FeDataBuffer* buf, size_t at, u64 data);

#include "iron/passes/passes.h"
#include "iron/codegen/mach.h"