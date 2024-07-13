#pragma once

#include "orbit.h"
#include "alloc.h"
#include "arena.h"

typedef struct FeModule FeModule;
typedef struct FePass   FePass;

typedef struct FeType FeType;
typedef struct FeInst      FeInst;
typedef        FeInst*     FeInstPTR;

typedef struct FeFunction     FeFunction;
typedef struct FeFunctionItem FeFunctionItem;
typedef struct FeData         FeData;
typedef struct FeSymbol       FeSymbol;
typedef struct FeBasicBlock   FeBasicBlock;

typedef struct FeVReg           FeVReg;
typedef struct FeAsmBuffer      FeAsmBuffer;

typedef struct FeAsm            FeAsm;
typedef struct FeAsmInst        FeAsmInst;
typedef struct FeAsmInline      FeAsmInline;
typedef struct FeAsmLocalLabel  FeAsmLocalLabel;
typedef struct FeAsmSymbolLabel FeAsmSymbolLabel;
typedef struct FeAsmFuncBegin   FeAsmFuncBegin;
typedef struct FeAsmFuncEnd     FeAsmFuncEnd;
typedef struct FeAsmData        FeAsmData;

typedef struct FeArchInstInfo      FeArchInstInfo;
typedef struct FeArchRegisterInfo  FeArchRegisterInfo;
typedef struct FeArchRegisterClass FeArchRegisterClass;
typedef struct FeArchAsmSyntaxInfo FeArchAsmSyntaxInfo;
typedef struct FeArchInfo          FeArchInfo;

#define AS(ptr, type) ((type*)(ptr))

typedef struct FeModule {
    string name;

    FeFunction** functions;
    FeData** globals;

    u32 functions_len;
    u32 globals_len;

    struct {
        FeSymbol** at;
        size_t len;
        size_t cap;

        Arena alloca;
    } symtab; // symbol table

    struct {
        FeType** at;
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

    FeAsmBuffer* assembly;

} FeModule;

char* random_string(int len);

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
    FE_VOID,

    FE_BOOL,

    FE_PTR,

    FE_U8,
    FE_U16,
    FE_U32,
    FE_U64,

    FE_I8,
    FE_I16,
    FE_I32,
    FE_I64,

    FE_F16,
    FE_F32,
    FE_F64,

    FE_AGGREGATE,
    FE_ARRAY,
};

typedef struct FeType {
    u8 kind;
    u32 number;

    // u32 align;
    // u64 size;

    union {
    
        struct {
            u64 len;
            FeType* fields[]; // flexible array member
        } aggregate;

        struct {
            u64 len;
            FeType* sub;
        } array;
    };
} FeType;

enum {
    FE_VIS_GLOBAL,
    FE_VIS_LOCAL,
    FE_VIS_WEAK,
};

typedef struct FeSymbol {
    string name;
    union {
        void* ref;
        FeFunction* function;
        FeData* data;
    };
    bool is_function;
    bool is_extern;
    u8 visibility;
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
            u32 data_len;
            bool zeroed;
        };
        FeSymbol* symref;

        u8  d8;
        u16 d16;
        u32 d32;
        u64 d64;
    };

    u8 kind;
    bool read_only;
} FeData;

#define FE_FN_ALLOCA_BLOCK_SIZE 0x1000

typedef struct FeStackObject {
    FeType* t;
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

    FeFunctionItem** params;
    FeFunctionItem** returns;

    u16 params_len;
    u16 returns_len;

    u32 entry_idx; // 0 most of the time, but not guaranteed
    // u32 exit_idx;

    Arena alloca;
} FeFunction;

typedef struct FeFunctionItem {

    FeType* T;

    // if its an aggregate, this is exposed as a pointer marked "by_val" so that
    // calling conventions work without needing target-specific information.
    bool by_val_aggregate;
} FeFunctionItem;

typedef struct FeBasicBlock {
    FeInst** at;
    u64 len;
    u64 cap;

    string name;

    FeBasicBlock* idom; // immediate dominator
    FeBasicBlock** domset; // all blocks that this block dominates (including self)

    FeBasicBlock** outgoing;
    FeBasicBlock** incoming;
    u16 out_len;
    u16 in_len;
    u32 domset_len;

    u64 flags; // for misc use
} FeBasicBlock;

enum {
    FE_INST_INVALID,
    FE_INST_ELIMINATED, // an AIR element that has been "deleted".

    // FeBinop
    FE_INST_ADD,
    FE_INST_SUB,
    FE_INST_MUL,
    FE_INST_DIV,

    // FeBinop
    FE_INST_AND,
    FE_INST_OR,
    FE_INST_NOR,
    FE_INST_XOR,
    FE_INST_SHL,
    FE_INST_ASR,
    FE_INST_LSR,

    // FeCast
    FE_INST_CAST,

    // FeStackAddr
    FE_INST_STACKADDR,

    // FeGetFieldPtr
    FE_INST_GETFIELDPTR,

    // FeGetIndexPtr
    FE_INST_GETINDEXPTR,

    // FeLoad
    FE_INST_LOAD,
    FE_INST_VOL_LOAD,

    // FeStore
    FE_INST_STORE,
    FE_INST_VOL_STORE,

    // FeConst
    FE_INST_CONST,
    // FeLoadSymbol
    FE_INST_LOADSYMBOL,

    // FeMov
    FE_INST_MOV,
    // FePhi
    FE_INST_PHI,

    // FeBranch
    FE_INST_BRANCH,
    // FeJump
    FE_INST_JUMP,

    // FeParamVal
    FE_INST_PARAMVAL,
    // FeReturnVal
    FE_INST_RETURNVAL,
    // FeReturn
    FE_INST_RETURN,

    _FE_INST_COUNT,
};

// basic AIR structure
typedef struct FeInst {
    FeType* T;
    FeBasicBlock* bb;
    u32 number;
    u16 use_count;
    u8 tag;
} FeInst;

typedef struct FeBinop {
    FeInst base;

    FeInst* lhs;
    FeInst* rhs;
} FeBinop;

typedef struct FeCast {
    FeInst base;

    FeType* to;
    FeInst* source;
} FeCast;

typedef struct FeStackAddr {
    FeInst base;

    FeStackObject* object;
} FeStackAddr;

// used for struct accesses
typedef struct FeGetFieldPtr {
    FeInst base;

    FeInst* source;
    u32 index;
} FeGetFieldPtr;

// used for array accesses
typedef struct FeGetIndexPtr {
    FeInst base;

    FeInst* source;
    FeInst* index;
} FeGetIndexPtr;

typedef struct FeLoad {
    FeInst base;

    FeInst* location;
} FeLoad;

typedef struct FeStore {
    FeInst base;

    FeInst* location;
    FeInst* value;
} FeStore;

typedef struct FeConst {
    FeInst base;

    union {
        bool bool;
        
        i8  i8;
        i16 i16;
        i32 i32;
        i64 i64;

        u8  u8;
        u16 u16;
        u32 u32;
        u64 u64;

        f16 f16;
        f32 f32;
        f64 f64;
    };
} FeConst;

typedef struct FeLoadSymbol {
    FeInst base;
    
    FeSymbol* sym;
} FeLoadSymbol;

typedef struct FeMov {
    FeInst base;

    FeInst* source;
} FeMov;

typedef struct FePhi {
    FeInst base;

    FeInst** sources;
    FeBasicBlock** source_BBs;
    u32 len;
} FePhi;

typedef struct FeJump {
    FeInst base;

    FeBasicBlock* dest;
} FeJump;

enum {
    COND_LT,    // <
    COND_GT,    // >
    COND_LE,    // >=
    COND_GE,    // <=
    COND_EQ,    // ==
    COND_NE,    // !=
};

typedef struct FeBranch {
    FeInst base;

    u8 cond;

    FeInst* lhs;
    FeInst* rhs;

    FeBasicBlock* if_true;
    FeBasicBlock* if_false;
} FeBranch;

// get value from register parameter OR the pointer to a stack parameter.
// if register, lifetime of the register starts from the start of the entry 
// basic block and continues to this node.
// MUST BE THE FIRST INSTRUCTION IN THE ENTRY BLOCK OR IN A SEQUENCE OF 
// OTHER FeParamVal INSTRUCTIONS
// BECAUSE OF HOW THE REG ALLOCATOR USES THIS TO CONSTRUCT MACHINE REGISTER LIFETIMES
// ALONGSIDE VIRTUAL REGISTER LIFETIMES
typedef struct FeParamVal {
    FeInst base;

    u32 param_idx;
} FeParamVal;

// set register return val
typedef struct FeReturnVal {
    FeInst base;
    FeInst* source;

    u32 return_idx;
} FeReturnVal;

typedef struct FeReturn {
    FeInst base;
} FeReturn;

extern const size_t air_sizes[];

FeModule*     fe_new_module(string name);
FeType*       fe_type(FeModule* m, u8 kind, u64 len);
FeFunction*   fe_new_function(FeModule* mod, FeSymbol* sym);
FeBasicBlock* fe_new_basic_block(FeFunction* fn, string name);
FeData*     fe_new_data(FeModule* mod, FeSymbol* sym, bool read_only);
FeSymbol*     fe_new_symbol(FeModule* mod, string name, u8 visibility);
FeSymbol*     fe_find_symbol(FeModule* mod, string name);
FeSymbol*     fe_find_or_new_symbol(FeModule* mod, string name, u8 visibility);

FeStackObject* fe_new_stackobject(FeFunction* f, FeType* t);
void fe_set_func_params(FeFunction* f, u16 count, ...);
void fe_set_func_returns(FeFunction* f, u16 count, ...);
u32  fe_bb_index(FeFunction* fn, FeBasicBlock* bb);
void fe_set_data_bytes(FeData* data, u8* bytes, u32 data_len, bool zeroed);
void fe_set_data_as_symref(FeData* data, FeSymbol* symref);

FeInst* fe_append(FeBasicBlock* bb, FeInst* ir);
FeInst* fe_inst(FeFunction* f, u8 type);

FeInst* fe_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs);
FeInst* fe_cast(FeFunction* f, FeInst* source, FeType* to);
FeInst* fe_stackoffset(FeFunction* f, FeStackObject* obj);
FeInst* fe_getfieldptr(FeFunction* f, u32 index, FeInst* source);
FeInst* fe_getindexptr(FeFunction* f, FeInst* index, FeInst* source);
FeInst* fe_load(FeFunction* f, FeInst* location, bool is_vol);
FeInst* fe_store(FeFunction* f, FeInst* location, FeInst* value, bool is_vol);
FeInst* fe_const(FeFunction* f);
FeInst* fe_loadsymbol(FeFunction* f, FeSymbol* symbol);
FeInst* fe_mov(FeFunction* f, FeInst* source);
FeInst* fe_phi(FeFunction* f, u32 count, ...);
FeInst* fe_jump(FeFunction* f, FeBasicBlock* dest);
FeInst* fe_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false);
FeInst* fe_paramval(FeFunction* f, u32 param);
FeInst* fe_returnval(FeFunction* f, u32 param, FeInst* source);
FeInst* fe_return(FeFunction* f);

void   fe_add_phi_source(FePhi* phi, FeInst* source, FeBasicBlock* source_block);
void   fe_move_inst(FeBasicBlock* bb, u64 to, u64 from);
string fe_emit_textual_ir(FeModule* am);

// ASSEMBLY SHIT

#define FE_PHYS_UNASSIGNED (UINT32_MAX)

enum {
    FE_VSPEC_NONE,
    FE_VSPEC_PARAMVAL, // extend this register's liveness to the beginning of the program
    FE_VSPEC_RETURNVAL, // extend this register's liveness to the end of the program

    FE_VSPEC_CALLPARAMVAL, // extend this register's liveness to the next call-classified instruction
    FE_VSPEC_CALLRETURNVAL, // extend this register's liveness to the nearest previous call-classified instruction
};

typedef struct FeVReg {
    // index of real register into regclass array (if FE_PHYS_UNASSIGNED, it has not been assigned yet)
    u32 real;

    u32 hint;

    // register class to pick from when assigning a machine register
    u32 required_regclass;

    // any special handling information
    u8 special;
} FeVReg;

enum {
    FE_IMM_I64,
    FE_IMM_U64,

    FE_IMM_F64,
    FE_IMM_F32,
    FE_IMM_F16,
    FE_IMM_SYM_ABSOLUTE,
    FE_IMM_SYM_RELATIVE,
};

typedef struct FeImmediate {
    union {
        i64 i64;
        u64 u64;

        f64 f64; // the float stuff isnt really useful i think
        f32 f32;
        f16 f16;
        FeArchAsmSyntaxInfo* sym;
    };
    u8 kind;
} FeImmediate;

#define _FE_ASM_BASE \
    u8 kind;

enum {
    FE_ASM_INST = 1,
    FE_ASM_JUMP_PATTERN,
    FE_ASM_INLINE,
    FE_ASM_LOCAL_LABEL,
    FE_ASM_SYMBOL_LABEL,
    FE_ASM_FUNC_BEGIN,
    FE_ASM_FUNC_END,
    FE_ASM_DATA,
};

typedef struct FeAsm {
    _FE_ASM_BASE
} FeAsm;

typedef struct FeAsmInst {
    _FE_ASM_BASE

    // input virtual registers, length dictated by its template
    FeVReg** ins;

    // output virtual registers, length dictated by its template
    FeVReg** outs;

    // immediate values, length dictated by its template
    FeImmediate* imms;

    // instruction kind information
    FeArchInstInfo* template;
} FeAsmInst;

// inline data structure for control-flow edges.
typedef struct FeAsmJumpPattern {
    _FE_ASM_BASE

    // source is implicitly the previous instruction
    FeAsm* dst; // where is it jumping?
    bool is_cond; // is it conditional (could control flow pass through this) ?
} FeAsmJumpPattern;

// textual inline assembly.
typedef struct FeAsmInline {
    _FE_ASM_BASE

    u16 ins_len;
    u16 outs_len;
    u16 interns_len;

    // input virtual registers
    FeVReg** ins;

    // output virtual registers
    FeVReg** outs;

    // internal virtual registers, used inside the asm block
    FeVReg** interns;

    string text;

} FeAsmInline;

// a lable that is not visible on the global level.
typedef struct FeAsmLocalLabel {
    _FE_ASM_BASE
    
    string text;
} FeAsmLocalLabel;

typedef struct FeAsmSymbolLabel {
    _FE_ASM_BASE
    
    FeSymbol* sym; // defines this symbol in the final assembly
} FeAsmSymbolLabel;

// tell the register allocator to start
typedef struct FeAsmFuncBegin {
    _FE_ASM_BASE

    FeFunction* func;
} FeAsmFuncBegin;

// tell the register allocator to stop, basically
typedef struct FeAsmFuncEnd {
    _FE_ASM_BASE

    FeAsmFuncBegin* open;
} FeAsmFuncEnd;

#undef _FE_ASM_BASE

// IR -> ASM outputs this
typedef struct FeAsmBuffer {
    FeAsm** at;
    size_t len;
    size_t cap;

    Arena alloca;

    FeArchInfo* target_arch;
} FeAsmBuffer;

FeAsm* fe_asm_append(FeModule* m, FeAsm* a);
FeAsm* fe_asm(FeModule* m, u8 kind);
FeAsm* fe_asm_inst(FeModule* m, FeArchInstInfo* template);

FeVReg* fe_new_vreg(FeModule* m, u32 regclass);

// fails if no target is provided
void       fe_codegen(FeModule* m);

string fe_export_assembly(FeModule* m);


/* TARGET DEFINITIONS AND INFORMATION */

// info about a register
typedef struct FeArchRegisterInfo {
    // thing to print in the asm
    string name; // if no alias index is provided, use this

    string* aliases; // if an alias index is provided, index into this
    u16 aliases_len;
} FeArchRegisterInfo;

typedef struct FeArchRegisterClass {
    // register list
    FeArchRegisterInfo* regs;
    u32 regs_len;

} FeArchRegisterClass;

enum {
    FE_ISPEC_NONE = 0, // no special handling
    FE_ISPEC_MOVE, // register allocator should try to copy-elide this
    FE_ISPEC_CALL, // register allocator needs to be careful about lifetimes over this
};

// instruction template, each MInst follows one.
typedef struct FeArchInstInfo {
    string asm_string;

    u16 num_imms;
    u16 num_ins;
    u16 num_outs;

    // instruction specific stuffs

    u8 bytesize;

    u8 special; // any special information/classification?
} FeArchInstInfo;

/*
    format strings for assembly - example:
        addr {out 0}, {in 0}, {in 1}

    with ins = {rb, rc} and outs = {ra}, this translates to:
        addr ra, rb, rc
    
    if some of the arguments are immediates, use 'imm'
        addr {out 0}, {in 0}, {imm 0}
*/

typedef struct FeArchAsmSyntaxInfo {
    string file_begin; // arbitrary text to put at the beginning
    string file_end; // arbitrary text to put at the end

    string d64;
    string d32;
    string d16;
    string d8;
    
    string zero; // filling a section with zero

    string string; // if NULL_STR, just use a bunch of `u8`
    char* escape_chars; // example: "\n\t"
    char* escape_codes; // example: "nt"
    u32 escapes_len; // example: 2

    string align;

    string label;
    string local_label; // for things like basic block labels.

    string bind_symbol_global;
    string bind_symbol_local;

} FeArchAsmSyntaxInfo;

// codegen target definition
typedef struct FeArchInfo {
    string name;

    FeArchRegisterClass* regclasses;
    u32 regclasses_len;

    FeArchInstInfo* insts;
    u32 insts_len;
    u8 inst_align;

    FeArchAsmSyntaxInfo* syntax_info;

} FeArchInfo;