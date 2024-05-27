#pragma once
#define ATLAS_AIR_H

#include "orbit.h"
#include "type.h"
#include "arena.h"
#include "entity.h"

typedef struct AIR AIR;
typedef AIR* AIR_PTR;
typedef struct AIR_BasicBlock AIR_BasicBlock;
typedef struct AIR_Module AIR_Module;
typedef struct AIR_Symbol AIR_Symbol;
typedef struct AIR_Global AIR_Global;
typedef struct AIR_Function AIR_Function;
typedef struct AIR_FuncItem AIR_FuncItem;

typedef struct AIR_Module {

    AIR_Function** functions;
    AIR_Global** globals;

    u32 functions_len;
    u32 globals_len;

    struct {
        AIR_Symbol** at;
        size_t len;
        size_t cap;
    } symtab;

} AIR_Module;

enum {
    AIR_SYM_GLOBAL,
    AIR_SYM_LOCAL,
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
    entity* e; // FIXME: remove this

    type* T;

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
    AIR_TRUNC,
    AIR_SEXT,
    AIR_ZEXT,

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
    type* T;
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

    type* to;
    AIR* source;
} AIR_Cast;

typedef struct AIR_StackAlloc {
    AIR base;

    type* alloctype;
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
    type* T;
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

AIR_Module      air_new_module();
AIR_Function*   air_new_function(AIR_Module* mod, AIR_Symbol* sym, bool global);
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
AIR* air_make_cast(AIR_Function* f, AIR* source, type* to);
AIR* air_make_stackalloc(AIR_Function* f, type* T);
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

string type_to_string(type* t);