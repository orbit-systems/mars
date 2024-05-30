#pragma once
#define ATLAS_H

#include "orbit.h"
#include "alloc.h"
#include "arena.h"

typedef struct AtlasModule AtlasModule;
typedef struct AtlasPass   AtlasPass;

typedef struct AIR_Type AIR_Type;
typedef struct AIR      AIR;
typedef        AIR*     AIR_Ptr;

typedef struct AIR_Module     AIR_Module;
typedef struct AIR_Function   AIR_Function;
typedef struct AIR_FuncItem   AIR_FuncItem;
typedef struct AIR_Global     AIR_Global;
typedef struct AIR_Symbol     AIR_Symbol;
typedef struct AIR_BasicBlock AIR_BasicBlock;

typedef struct TargetInstInfo      TargetInstInfo;
typedef struct TargetRegisterInfo  TargetRegisterInfo;
typedef struct TargetRegisterClass TargetRegisterClass;
typedef struct TargetFormatInfo    TargetFormatInfo;
typedef struct TargetInfo          TargetInfo;

typedef struct VReg        VReg;
typedef struct AsmInst     AsmInst;
typedef struct AsmBlock    AsmBlock;
typedef struct AsmSymbol   AsmSymbol;
typedef struct AsmGlobal   AsmGlobal;
typedef struct AsmFunction AsmFunction;
typedef struct AsmModule   AsmModule;

typedef struct AtlasModule {
    string name;

    AIR_Module* ir_module;
    AsmModule* asm_module;

    struct {
        AtlasPass** at;
        size_t len;
        size_t cap;

        bool cfg_up_to_date;
    } pass_queue;

} AtlasModule;

AtlasModule* atlas_new_module(string name, TargetInfo* target);

char* random_string(int len);

typedef struct AtlasPass {
    string name;
    union {
        void* raw_callback;
        void (*callback)(AtlasModule*);
    };

    // make sure CFG info is up-to-date before this pass runs
    bool requires_cfg;
    
    // mark the CFG information as out-of-date
    bool modifies_cfg;
} AtlasPass;

void atlas_sched_pass(AtlasModule* m, AtlasPass* p);
void atlas_sched_pass_at(AtlasModule* m, AtlasPass* p, int index);
void atlas_run_next_pass(AtlasModule* m);
void atlas_run_all_passes(AtlasModule* m);

enum {
    AIR_VOID,

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

typedef struct AIR_Type {
    u8 kind;
    u32 number;

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

void air_typegraph_init(AtlasModule* m);
AIR_Type* air_new_type(AtlasModule* m, u8 kind, u64 len);

typedef struct AIR_Module {
    AtlasModule* am;

    AIR_Function** functions;
    AIR_Global** globals;

    u32 functions_len;
    u32 globals_len;

    struct {
        AIR_Symbol** at;
        size_t len;
        size_t cap;
    } symtab;

    struct {
        AIR_Type** at;
        size_t len;
        size_t cap;

        arena alloca;
    } typegraph;

} AIR_Module;

enum {
    AIR_VIS_GLOBAL,
    AIR_VIS_LOCAL,
};

typedef struct AIR_Symbol {
    string name;
    union {
        void* ref;
        AIR_Function* function;
        AIR_Global* global;
    };
    bool is_function;
    bool is_extern;
    u8 visibility;
} AIR_Symbol;

typedef struct AIR_Global {
    AIR_Symbol* sym;

    bool is_symbol_ref;

    union {
        struct {
            u8* data;
            u32 data_len;
            bool zeroed;
        };
        AIR_Symbol* symref;
    };

    bool read_only;
} AIR_Global;

#define AIR_FN_ALLOCA_BLOCK_SIZE 0x1000

typedef struct AIR_Function {
    AIR_Module* mod;
    AIR_Symbol* sym;

    struct {
        AIR_BasicBlock** at;
        u32 len;
        u32 cap;
    } blocks;

    AIR_FuncItem** params;
    AIR_FuncItem** returns;

    u16 params_len;
    u16 returns_len;

    u32 entry_idx; // 0 most of the time, but not guaranteed
    // u32 exit_idx;

    arena alloca;
} AIR_Function;

typedef struct AIR_FuncItem {

    AIR_Type* T;

    // if its an aggregate, this is exposed as a pointer marked "by_val" so that
    // calling conventions work without needing target-specific information.
    bool by_val_aggregate;
} AIR_FuncItem;

typedef struct AIR_BasicBlock {
    AIR** at;
    u64 len;
    u64 cap;

    string name;

    AIR_BasicBlock* idom; // immediate dominator
    AIR_BasicBlock** domset; // all blocks that this block dominates (including self)

    AIR_BasicBlock** outgoing;
    AIR_BasicBlock** incoming;
    u16 out_len;
    u16 in_len;
    u32 domset_len;

    u64 flags; // for misc use
} AIR_BasicBlock;

enum {
    AIR_INVALID,
    AIR_ELIMINATED, // an AIR element that has been "deleted".

    // AIR_BinOp
    AIR_ADD,
    AIR_SUB,
    AIR_MUL,
    AIR_DIV,

    // AIR_BinOp
    AIR_AND,
    AIR_OR,
    AIR_NOR,
    AIR_XOR,
    AIR_SHL,
    AIR_ASR,
    AIR_LSR,

    // AIR_UnOp
    AIR_NOT,

    // AIR_Cast
    AIR_CAST,

    // AIR_StackAlloc
    AIR_STACKALLOC,

    // AIR_GetFieldPtr
    AIR_GETFIELDPTR,

    // AIR_GetIndexPtr
    AIR_GETINDEXPTR,

    // AIR_Load
    AIR_LOAD,
    AIR_VOL_LOAD,

    // AIR_Store
    AIR_STORE,
    AIR_VOL_STORE,

    // AIR_Const
    AIR_CONST,
    // AIR_LoadSymbol
    AIR_LOADSYMBOL,

    // AIR_Mov
    AIR_MOV,
    // AIR_Phi
    AIR_PHI,

    // AIR_Branch
    AIR_BRANCH,
    // AIR_Jump
    AIR_JUMP,

    // AIR_ParamVal
    AIR_PARAMVAL,
    // AIR_ReturnVal
    AIR_RETURNVAL,
    // AIR_Return
    AIR_RETURN,

    AIR_INSTR_COUNT,
};

// basic AIR structure
typedef struct AIR {
    AIR_Type* T;
    AIR_BasicBlock* bb;
    u32 number;
    u16 use_count;
    u8 tag;
} AIR;

typedef struct AIR_BinOp {
    AIR base;

    AIR* lhs;
    AIR* rhs;
} AIR_BinOp;

typedef struct AIR_Cast {
    AIR base;

    AIR_Type* to;
    AIR* source;
} AIR_Cast;

typedef struct AIR_StackAlloc {
    AIR base;

    AIR_Type* alloctype;
} AIR_StackAlloc;

// used for struct accesses
typedef struct AIR_GetFieldPtr {
    AIR base;

    u32 index;

    AIR* source;
} AIR_GetFieldPtr;

// used for array accesses
typedef struct AIR_GetIndexPtr {
    AIR base;

    AIR* index;

    AIR* source;
} AIR_GetIndexPtr;

typedef struct AIR_Load {
    AIR base;

    AIR* location;
} AIR_Load;

typedef struct AIR_Store {
    AIR base;

    AIR* location;
    AIR* value;
} AIR_Store;

typedef struct AIR_Const {
    AIR base;

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
} AIR_Const;

typedef struct AIR_LoadSymbol {
    AIR base;
    
    AIR_Symbol* sym;
} AIR_LoadSymbol;

typedef struct AIR_Mov {
    AIR base;

    AIR* source;
} AIR_Mov;

typedef struct AIR_Phi {
    AIR base;

    AIR** sources;
    AIR_BasicBlock** source_BBs;
    u32 len;
} AIR_Phi;

typedef struct AIR_Jump {
    AIR base;

    AIR_BasicBlock* dest;
} AIR_Jump;

enum {
    COND_LT,    // <
    COND_GT,    // >
    COND_LE,    // >=
    COND_GE,    // <=
    COND_EQ,    // ==
    COND_NE,    // !=
};

typedef struct AIR_Branch {
    AIR base;

    u8 cond;

    AIR* lhs;
    AIR* rhs;

    AIR_BasicBlock* if_true;
    AIR_BasicBlock* if_false;
} AIR_Branch;

// get value from register parameter OR the pointer to a stack parameter.
// if register, lifetime of the register starts from the start of the entry 
// basic block and continues to this node.
// MUST BE THE FIRST INSTRUCTION IN THE ENTRY BLOCK OR IN A SEQUENCE OF 
// OTHER AIR_ParamVal INSTRUCTIONS
// BECAUSE OF HOW THE REG ALLOCATOR USES THIS TO CONSTRUCT MACHINE REGISTER LIFETIMES
// ALONGSIDE VIRTUAL REGISTER LIFETIMES
typedef struct AIR_ParamVal {
    AIR base;

    u32 param_idx;
} AIR_ParamVal;

// set register return val
typedef struct AIR_ReturnVal {
    AIR base;
    AIR* source;

    u32 return_idx;
} AIR_ReturnVal;

typedef struct AIR_Return {
    AIR base;
} AIR_Return;

extern const size_t air_sizes[];

AIR_Module*     air_new_module(AtlasModule* am);
AIR_Function*   air_new_function(AIR_Module* mod, AIR_Symbol* sym, u8 visibility);
AIR_BasicBlock* air_new_basic_block(AIR_Function* fn, string name);
AIR_Global*     air_new_global(AIR_Module* mod, AIR_Symbol* sym, bool global, bool read_only);
AIR_Symbol*     air_new_symbol(AIR_Module* mod, string name, u8 visibility, bool function, void* ref);
AIR_Symbol*     air_find_symbol(AIR_Module* mod, string name);
AIR_Symbol*     air_find_or_new_symbol(AIR_Module* mod, string name, u8 visibility, bool function, void* ref);

void air_set_func_params(AIR_Function* f, u16 count, ...);
void air_set_func_returns(AIR_Function* f, u16 count, ...);
u32  air_bb_index(AIR_Function* fn, AIR_BasicBlock* bb);
void air_set_global_data(AIR_Global* global, u8* data, u32 data_len, bool zeroed);
void air_set_global_symref(AIR_Global* global, AIR_Symbol* symref);

AIR* air_add(AIR_BasicBlock* bb, AIR* ir);
AIR* air_make(AIR_Function* f, u8 type);
AIR* air_make_binop(AIR_Function* f, u8 type, AIR* lhs, AIR* rhs);
AIR* air_make_cast(AIR_Function* f, AIR* source, AIR_Type* to);
AIR* air_make_stackalloc(AIR_Function* f, AIR_Type* T);
AIR* air_make_getfieldptr(AIR_Function* f, u32 index, AIR* source);
AIR* air_make_getindexptr(AIR_Function* f, AIR* index, AIR* source);
AIR* air_make_load(AIR_Function* f, AIR* location, bool is_vol);
AIR* air_make_store(AIR_Function* f, AIR* location, AIR* value, bool is_vol);
AIR* air_make_const(AIR_Function* f);
AIR* air_make_loadsymbol(AIR_Function* f, AIR_Symbol* symbol);
AIR* air_make_mov(AIR_Function* f, AIR* source);
AIR* air_make_phi(AIR_Function* f, u32 count, ...);
AIR* air_make_jump(AIR_Function* f, AIR_BasicBlock* dest);
AIR* air_make_branch(AIR_Function* f, u8 cond, AIR* lhs, AIR* rhs, AIR_BasicBlock* if_true, AIR_BasicBlock* if_false);
AIR* air_make_paramval(AIR_Function* f, u32 param);
AIR* air_make_returnval(AIR_Function* f, u32 param, AIR* source);
AIR* air_make_return(AIR_Function* f);

void air_move_element(AIR_BasicBlock* bb, u64 to, u64 from);

void air_add_phi_source(AIR_Phi* phi, AIR* source, AIR_BasicBlock* source_block);

void air_print_module(AIR_Module* mod);
void air_print_function(AIR_Function* f);
void air_print_bb(AIR_BasicBlock* bb);
void air_print_ir(AIR* ir);


#define ATLAS_PHYS_UNASSIGNED (UINT32_MAX)

enum {
    VSPEC_NONE,
    VSPEC_PARAMVAL, // extend this register's liveness to the beginning of the program
    VSPEC_RETURNVAL, // extend this register's liveness to the end of the program

    VSPEC_CALLPARAMVAL, // extend this register's liveness to the next call-classified instruction
    VSPEC_CALLRETURNVAL, // extend this register's liveness to the nearest previous call-classified instruction
};

typedef struct VReg {
    // index of real register into regclass array (if ATLAS_PHYS_UNASSIGNED, it has not been assigned yet)
    u32 real;

    u32 hint;

    // register class to pick from when assigning a machine register
    u32 required_regclass;

    // any special handling information
    u8 special;
} VReg;

enum {
    IMM_I64,
    IMM_U64,

    IMM_F64,
    IMM_F32,
    IMM_F16,
    IMM_SYM,
};

typedef struct ImmediateVal {
    union {
        i64 i64;
        u64 u64;

        f64 f64; // the float stuff isnt really useful i think
        f32 f32;
        f16 f16;
        AsmSymbol* sym;
    };
    u8 kind;
} ImmediateVal;

typedef struct AsmInst {

    // input virtual registers, length dictated by its TargetInstInfo
    VReg** ins;

    // output virtual registers, length dictated by its TargetInstInfo
    VReg** outs;

    // immediate values, length dictated by its TargetInstInfo
    ImmediateVal* imms;

    // instruction kind information
    TargetInstInfo* template;
} AsmInst;

typedef struct AsmBlock {
    AsmInst** at;
    u32 len;
    u32 cap;

    string label;

    AsmFunction* f;
} AsmBlock;

typedef struct AsmFunction {
    AsmBlock** blocks; // in order
    u32 num_blocks;

    struct {
        VReg** at;
        u32 len;
        u32 cap;
    } vregs;

    AsmSymbol* sym;
    AsmModule* mod;
} AsmFunction;

enum {
    SYMBIND_GLOBAL,
    SYMBIND_LOCAL,
    // more probably later
};

typedef struct AsmSymbol {
    string name;

    u8 binding;

    union {
        AsmGlobal* glob;
        AsmFunction* func;
    };

} AsmSymbol;

enum {
    GLOBAL_INVALID,
    GLOBAL_U8,
    GLOBAL_U16,
    GLOBAL_U32,
    GLOBAL_U64,

    GLOBAL_I8,
    GLOBAL_I16,
    GLOBAL_I32,
    GLOBAL_I64,

    GLOBAL_ZEROES,
    GLOBAL_BYTES,
};

typedef struct AsmGlobal {
    AsmSymbol* sym;

    union {

        u8  u8;
        u16 u16;
        u32 u32;
        u64 u64;
        
        i8  i8;
        i16 i16;
        i32 i32;
        i64 i64;

        struct {
            u8* data; // only used with GLOBAL_BYTES
            u32 len; // only used with GLOBAL_BYTES and GLOBAL_ZEROES
        };
    };

    u8 type;
} AsmGlobal;

typedef struct AsmModule {
    AtlasModule* am;

    TargetInfo* target;

    AsmGlobal** globals;
    u32 globals_len;

    AsmFunction** functions;
    u32 functions_len;

    arena alloca;
} AsmModule;

/* TARGET DEFINITIONS AND INFORMATION */

// info about a register
typedef struct TargetRegisterInfo {
    // thing to print in the asm
    string name;
} TargetRegisterInfo;

typedef struct TargetRegisterClass {
    // register list
    TargetRegisterInfo* regs;
    u32 regs_len;

} TargetRegisterClass;

typedef struct TargetCallingConv {
    TargetRegisterInfo** param_regs;
    u16 param_regs_len;

    TargetRegisterInfo** return_regs;
    u16 return_regs_len;
} TargetCallingConv;

enum {
    ISPEC_NONE = 0, // no special handling
    ISPEC_MOVE, // register allocator should try to copy-elide this
    ISPEC_CALL, // register allocator needs to be careful about lifetimes over this
};

// instruction template, each MInst follows one.
typedef struct TargetInstInfo {
    string asm_string;

    u16 num_imms;
    u16 num_ins;
    u16 num_outs;

    // instruction specific stuffs

    u8 special; // any special information/classification?
} TargetInstInfo;

/*
    format strings for assembly - example:
        addr {out 0}, {in 0}, {in 1}

    with ins = {rb, rc} and outs = {ra}, this translates to:
        addr ra, rb, rc
    
    if some of the arguments are immediates, use 'imm'
        addr {out 0}, {in 0}, {imm 0}
*/

typedef struct TargetFormatInfo {
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

} TargetFormatInfo;

// codegen target definition
typedef struct TargetInfo {
    string name;

    TargetRegisterClass* regclasses;
    u32 regclasses_len;

    TargetInstInfo* insts;
    u32 insts_len;
    u8 inst_align;

    TargetFormatInfo* format_info;

} TargetInfo;

AsmModule*   asm_new_module(TargetInfo* target);
AsmFunction* asm_new_function(AsmModule* m, AsmSymbol* sym);
AsmGlobal*   asm_new_global(AsmModule* m, AsmSymbol* sym);
AsmBlock*    asm_new_block(AsmFunction* f, string label);
AsmInst*     asm_add_inst(AsmBlock* b, AsmInst* inst);
AsmInst*     asm_new_inst(AsmModule* m, u32 template);
AsmSymbol*   air_sym_to_asm_sym(AsmModule* m, AIR_Symbol* sym);

VReg*        asm_new_vreg(AsmModule* m, AsmFunction* f, u32 regclass);