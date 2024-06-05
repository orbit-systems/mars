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
typedef struct FeGlobal       FeGlobal;
typedef struct FeSymbol       FeSymbol;
typedef struct FeBasicBlock   FeBasicBlock;

typedef struct FeArchInstInfo      FeArchInstInfo;
typedef struct FeArchRegisterInfo  FeArchRegisterInfo;
typedef struct FeArchRegisterClass FeArchRegisterClass;
typedef struct FeArchFormatInfo    FeArchFormatInfo;
typedef struct FeArchInfo          FeArchInfo;

typedef struct FeVReg           FeVReg;
typedef struct FeAsm            FeAsm;
typedef struct FeAsmInst        FeAsmInst;
typedef struct FeAsmLocalLabel  FeAsmLocalLabel;
typedef struct FeAsmGlobalLabel FeAsmGlobalLabel;
typedef struct FeAsmFuncBegin   FeAsmFuncBegin;
typedef struct FeAsmDataBegin   FeAsmDataBegin;

typedef struct FeModule {
    string name;

    FeFunction** functions;
    FeGlobal** globals;

    u32 functions_len;
    u32 globals_len;

    struct {
        FeSymbol** at;
        size_t len;
        size_t cap;

        arena alloca;
    } symtab;

    struct {
        FeType** at;
        size_t len;
        size_t cap;

        arena alloca;
    } typegraph;

    struct {
        FePass** at;
        size_t len;
        size_t cap;

        bool cfg_up_to_date;
    } pass_queue;

    struct {
        FeAsm** at;
        size_t len;
        size_t cap;

        arena alloca;
    } assembly;

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

void atlas_sched_pass(FeModule* m, FePass* p);
void atlas_sched_pass_at(FeModule* m, FePass* p, int index);
void atlas_run_next_pass(FeModule* m, bool printout);
void atlas_run_all_passes(FeModule* m, bool printout);

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

void air_typegraph_init(FeModule* m);
FeType* air_new_type(FeModule* m, u8 kind, u64 len);

enum {
    FE_VIS_GLOBAL,
    FE_VIS_LOCAL,
};

typedef struct FeSymbol {
    string name;
    union {
        void* ref;
        FeFunction* function;
        FeGlobal* global;
    };
    bool is_function;
    bool is_extern;
    u8 visibility;
} FeSymbol;

typedef struct FeGlobal {
    FeSymbol* sym;

    bool is_symbol_ref;

    union {
        struct {
            u8* data;
            u32 data_len;
            bool zeroed;
        };
        FeSymbol* symref;
    };

    bool read_only;
} FeGlobal;

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

    arena alloca;
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
    FE_INVALID,
    FE_ELIMINATED, // an AIR element that has been "deleted".

    // FeBinop
    FE_ADD,
    FE_SUB,
    FE_MUL,
    FE_DIV,

    // FeBinop
    FE_AND,
    FE_OR,
    FE_NOR,
    FE_XOR,
    FE_SHL,
    FE_ASR,
    FE_LSR,

    // FE_UnOp
    FE_NOT,

    // FeCast
    FE_CAST,

    // FeStackOffset
    FE_STACKOFFSET,

    // FeGetFieldPtr
    FE_GETFIELDPTR,

    // FeGetIndexPtr
    FE_GETINDEXPTR,

    // FeLoad
    FE_LOAD,
    FE_VOL_LOAD,

    // FeStore
    FE_STORE,
    FE_VOL_STORE,

    // FeConst
    FE_CONST,
    // FeLoadSymbol
    FE_LOADSYMBOL,

    // FeMov
    FE_MOV,
    // FePhi
    FE_PHI,

    // FeBranch
    FE_BRANCH,
    // FeJump
    FE_JUMP,

    // FeParamVal
    FE_PARAMVAL,
    // FeReturnVal
    FE_RETURNVAL,
    // FeReturn
    FE_RETURN,

    FE_INSTR_COUNT,
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

typedef struct FeStackOffset {
    FeInst base;

    FeStackObject* object;
} FeStackOffset;

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

FeModule*     fe_new_module(FeModule* mod);
FeFunction*   air_new_function(FeModule* mod, FeSymbol* sym, u8 visibility);
FeBasicBlock* air_new_basic_block(FeFunction* fn, string name);
FeGlobal*     air_new_global(FeModule* mod, FeSymbol* sym, bool global, bool read_only);
FeSymbol*     air_new_symbol(FeModule* mod, string name, u8 visibility, bool function, void* ref);
FeSymbol*     air_find_symbol(FeModule* mod, string name);
FeSymbol*     air_find_or_new_symbol(FeModule* mod, string name, u8 visibility, bool function, void* ref);

FeStackObject* air_new_stackobject(FeFunction* f, FeType* t);
void air_set_func_params(FeFunction* f, u16 count, ...);
void air_set_func_returns(FeFunction* f, u16 count, ...);
u32  air_bb_index(FeFunction* fn, FeBasicBlock* bb);
void air_set_global_data(FeGlobal* global, u8* data, u32 data_len, bool zeroed);
void air_set_global_symref(FeGlobal* global, FeSymbol* symref);

FeInst* air_add(FeBasicBlock* bb, FeInst* ir);
FeInst* air_make(FeFunction* f, u8 type);
FeInst* air_make_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs);
FeInst* air_make_cast(FeFunction* f, FeInst* source, FeType* to);
FeInst* air_make_stackoffset(FeFunction* f, FeStackObject* obj);
FeInst* air_make_getfieldptr(FeFunction* f, u32 index, FeInst* source);
FeInst* air_make_getindexptr(FeFunction* f, FeInst* index, FeInst* source);
FeInst* air_make_load(FeFunction* f, FeInst* location, bool is_vol);
FeInst* air_make_store(FeFunction* f, FeInst* location, FeInst* value, bool is_vol);
FeInst* air_make_const(FeFunction* f);
FeInst* air_make_loadsymbol(FeFunction* f, FeSymbol* symbol);
FeInst* air_make_mov(FeFunction* f, FeInst* source);
FeInst* air_make_phi(FeFunction* f, u32 count, ...);
FeInst* air_make_jump(FeFunction* f, FeBasicBlock* dest);
FeInst* air_make_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false);
FeInst* air_make_paramval(FeFunction* f, u32 param);
FeInst* air_make_returnval(FeFunction* f, u32 param, FeInst* source);
FeInst* air_make_return(FeFunction* f);

void air_move_element(FeBasicBlock* bb, u64 to, u64 from);

void air_add_phi_source(FePhi* phi, FeInst* source, FeBasicBlock* source_block);

void air_print_module(FeModule* mod);
void air_print_function(FeFunction* f);
void air_print_bb(FeBasicBlock* bb);
void air_print_ir(FeInst* ir);

string air_textual_emit(FeModule* am);

#define FE_PHYS_UNASSIGNED (UINT32_MAX)

enum {
    VSPEC_NONE,
    VSPEC_PARAMVAL, // extend this register's liveness to the beginning of the program
    VSPEC_RETURNVAL, // extend this register's liveness to the end of the program

    VSPEC_CALLPARAMVAL, // extend this register's liveness to the next call-classified instruction
    VSPEC_CALLRETURNVAL, // extend this register's liveness to the nearest previous call-classified instruction
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
    IMM_I64,
    IMM_U64,

    IMM_F64,
    IMM_F32,
    IMM_F16,
    IMM_SYM,
};

typedef struct FeImmediate {
    union {
        i64 i64;
        u64 u64;

        f64 f64; // the float stuff isnt really useful i think
        f32 f32;
        f16 f16;
        FeArchFormatInfo* sym;
    };
    u8 kind;
} FeImmediate;

typedef struct FeAsmInst {

    // input virtual registers, length dictated by its FeArchInstInfo
    FeVReg** ins;

    // output virtual registers, length dictated by its FeArchInstInfo
    FeVReg** outs;

    // immediate values, length dictated by its FeArchInstInfo
    FeImmediate* imms;

    // instruction kind information
    FeArchInstInfo* template;
} FeAsmInst;

/* TARGET DEFINITIONS AND INFORMATION */

// info about a register
typedef struct FeArchRegisterInfo {
    // thing to print in the asm
    string name;
} FeArchRegisterInfo;

typedef struct FeArchRegisterClass {
    // register list
    FeArchRegisterInfo* regs;
    u32 regs_len;

} FeArchRegisterClass;

typedef struct TargetCallingConv {
    FeArchRegisterInfo** param_regs;
    u16 param_regs_len;

    FeArchRegisterInfo** return_regs;
    u16 return_regs_len;
} TargetCallingConv;

enum {
    ISPEC_NONE = 0, // no special handling
    ISPEC_MOVE, // register allocator should try to copy-elide this
    ISPEC_CALL, // register allocator needs to be careful about lifetimes over this
};

// instruction template, each MInst follows one.
typedef struct FeArchInstInfo {
    string asm_string;

    u16 num_imms;
    u16 num_ins;
    u16 num_outs;

    // instruction specific stuffs

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

typedef struct FeArchFormatInfo {
    string file_begin; // arbitrary text to put at the beginning
    string file_end; // arbitrary text to put at the end

    string u64;
    string u32;
    string u16;
    string u8;

    string i64;
    string i32;
    string i16;
    string i8;
    
    string zero; // filling a section with zero

    string string; // if NULL_STR, just use a bunch of `u8`
    char* escape_chars; // example: "\n\t"
    char* escape_codes; // example: "nt"
    u32 escapes_len; // example: 2

    string align;

    string label;
    string block_label; // for things like basic block labels.

    string bind_symbol_global;
    string bind_symbol_local;

    string begin_code_section;
    string begin_data_section;
    string begin_rodata_section;
    string begin_bss_section;

} FeArchFormatInfo;

// codegen target definition
typedef struct FeArchInfo {
    string name;

    FeArchRegisterClass* regclasses;
    u32 regclasses_len;

    FeArchInstInfo* insts;
    u32 insts_len;
    u8 inst_align;

    FeArchFormatInfo* format_info;

} FeArchInfo;

FeAsm*     fe_asm_add(FeModule* m, FeAsm* inst);
FeAsm*     fe_asm_inst(FeModule* m, FeArchInstInfo template);

FeVReg*    fe_new_vreg(FeModule* m, u32 regclass);