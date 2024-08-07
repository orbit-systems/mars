#pragma once

#include "common/orbit.h"
#include "common/alloc.h"
#include "common/arena.h"

#define FE_VERSION_MAJOR 0
#define FE_VERSION_MINOR 1
#define FE_VERSION_PATCH 0

typedef struct FeModule FeModule;
typedef struct FePass   FePass;

typedef struct FeType  FeType;
typedef struct FeInst  FeInst;
typedef        FeInst* FeInstPTR;

da_typedef(FeInstPTR);

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

    struct {
        u16 arch;
        u16 system;
        u16 product;
        void* arch_config;
    } target;

    FeAsmBuffer* assembly;

} FeModule;

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
    _FE_TYPE_SIMPLE_BEGIN,

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
    FE_BIND_IMPORT,
    FE_BIND_EXPORT,
    FE_BIND_LOCAL,
    FE_BIND_EXPORT_WEAK,
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

    Arena alloca;
} FeFunction;

typedef struct FeFunctionItem {

    FeType* type;

    // if its an aggregate, this is exposed as a pointer marked "by_val" so that
    // calling conventions work without needing target-specific information.
    FeType* by_value;
} FeFunctionItem;

typedef struct FeBasicBlock {
    FeInst** at;
    u64 len;
    u64 cap;

    string name;

    FeFunction* function;

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
    FE_INST_ELIMINATED, // an FeInst that has been "deleted".

    _FE_INST_NO_SIDE_EFFECTS_BEGIN,
    // FeBinop
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
    FE_INST_STACKADDR,

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

    // FeInstStore
    FE_INST_STORE,
    FE_INST_VOL_STORE,

    // FeInstBranch
    FE_INST_BRANCH,
    // FeInstJump
    FE_INST_JUMP,

    // FeInstReturnval
    FE_INST_RETURNVAL,
    // FeInstReturn
    FE_INST_RETURN,

    _FE_INST_MAX,
};

// basic AIR structure
typedef struct FeInst {
    FeType* type;
    FeBasicBlock* bb;
    u32 number;
    u16 use_count;
    u8 kind;
} FeInst;

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
    u32 len;
} FeInstPhi;

typedef struct FeInstJump {
    FeInst base;

    FeBasicBlock* dest;
} FeInstJump;

enum {
    FE_COND_NONE,
    FE_COND_ILT,   // signed <
    FE_COND_IGT,   // signed >
    FE_COND_ILE,   // signed >=
    FE_COND_IGE,   // signed <=
    FE_COND_ULT,   // unsigned <
    FE_COND_UGT,   // unsigned >
    FE_COND_ULE,   // unsigned >=
    FE_COND_UGE,   // unsigned <=
    FE_COND_EQ,    // ==
    FE_COND_NE,    // !=
};

typedef struct FeInstBranch {
    FeInst base;

    u8 cond;

    FeInst* lhs;
    FeInst* rhs;

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

    u32 param_idx;
} FeInstParamVal;

// set register return val
typedef struct FeInstReturnval {
    FeInst base;
    FeInst* source;

    u32 return_idx;
} FeInstReturnval;

typedef struct FeInstReturn {
    FeInst base;
} FeInstReturn;

extern const size_t fe_inst_sizes[];

FeModule*     fe_new_module(string name);
FeType*       fe_type(FeModule* m, u8 kind);
FeType*       fe_type_array(FeModule* m, FeType* subtype, u64 len);
FeType*       fe_type_aggregate(FeModule* m, u64 len);
FeFunction*   fe_new_function(FeModule* mod, FeSymbol* sym);
FeBasicBlock* fe_new_basic_block(FeFunction* fn, string name);
FeData*       fe_new_data(FeModule* mod, FeSymbol* sym, bool read_only);
FeSymbol*     fe_new_symbol(FeModule* mod, string name, u8 binding);
FeSymbol*     fe_find_symbol(FeModule* mod, string name);
FeSymbol*     fe_find_or_new_symbol(FeModule* mod, string name, u8 binding);

void fe_set_target(FeModule* m, u16 arch, u16 system, u16 product);
void fe_set_target_config(FeModule* m, void* meta);

void fe_destroy_module(FeModule* m);
void fe_destroy_function(FeFunction* f);
void fe_destroy_basic_block(FeBasicBlock* bb);
void fe_selftest();

FeStackObject* fe_new_stackobject(FeFunction* f, FeType* t);
void fe_set_func_params(FeFunction* f, u16 count, ...);
void fe_set_func_returns(FeFunction* f, u16 count, ...);
u32  fe_bb_index(FeFunction* fn, FeBasicBlock* bb);
void fe_set_data_bytes(FeData* data, u8* bytes, u32 data_len, bool zeroed);
void fe_set_data_symref(FeData* data, FeSymbol* symref);

FeInst* fe_append(FeBasicBlock* bb, FeInst* ir);
FeInst* fe_insert(FeBasicBlock* bb, FeInst* inst, i64 index);
FeInst* fe_insert_before(FeBasicBlock* bb, FeInst* inst, FeInst* ref);
FeInst* fe_insert_after(FeBasicBlock* bb, FeInst* inst, FeInst* ref);
i64     fe_index_of_inst(FeBasicBlock* bb, FeInst* inst);
void    fe_move(FeBasicBlock* bb, u64 to, u64 from);
void    fe_rewrite_uses(FeFunction* f, FeInst* source, FeInst* dest);
void    fe_add_uses_to_worklist(FeFunction* f, FeInst* source, da(FeInstPTR)* worklist);

FeInst* fe_inst(FeFunction* f, u8 type);
FeInst* fe_inst_binop(FeFunction* f, u8 type, FeInst* lhs, FeInst* rhs);
FeInst* fe_inst_unop(FeFunction* f, u8 type, FeInst* source);
FeInst* fe_inst_stackaddr(FeFunction* f, FeStackObject* obj);
FeInst* fe_inst_getfieldptr(FeFunction* f, u32 index, FeInst* source);
FeInst* fe_inst_getindexptr(FeFunction* f, FeInst* index, FeInst* source);
FeInst* fe_inst_load(FeFunction* f, FeInst* location, bool is_vol);
FeInst* fe_inst_store(FeFunction* f, FeInst* location, FeInst* value, bool is_vol);
FeInst* fe_inst_const(FeFunction* f);
FeInst* fe_inst_load_symbol(FeFunction* f, FeSymbol* symbol);
FeInst* fe_inst_mov(FeFunction* f, FeInst* source);
FeInst* fe_inst_phi(FeFunction* f, u32 count, ...);
FeInst* fe_inst_jump(FeFunction* f, FeBasicBlock* dest);
FeInst* fe_inst_branch(FeFunction* f, u8 cond, FeInst* lhs, FeInst* rhs, FeBasicBlock* if_true, FeBasicBlock* if_false);
FeInst* fe_inst_paramval(FeFunction* f, u32 param);
FeInst* fe_inst_returnval(FeFunction* f, u32 param, FeInst* source);
FeInst* fe_inst_return(FeFunction* f);

void   fe_add_phi_source(FeInstPhi* phi, FeInst* source, FeBasicBlock* source_block);

string fe_emit_ir(FeModule* m, bool fancy_whitespace);
FeModule* fe_read_ir(string text);

string fe_emit_c(FeModule* m);

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
    FE_ASM_NONE = 0,
    FE_ASM_INST,            // an assembly code instruction.
    FE_ASM_JUMP_PATTERN,    // indicates a control-flow jump.
    FE_ASM_INLINE,          // arbitrary assembly text (with optional register interaction)
    FE_ASM_LOCAL_LABEL,     // a label for internal use, such as a basic block.
    FE_ASM_SYMBOL_LABEL,    // a label that defines a symbol.
    FE_ASM_FUNC_BEGIN,      // marks the beginning of a function.
    FE_ASM_FUNC_END,        // marks the end of a function.
    FE_ASM_DATA,            // creates a chunk of arbitrary data.
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

// a lable that is only visible within this function/data block.
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

typedef struct FeAsmData {
    _FE_ASM_BASE

    FeData* data;
} FeAsmData;

#undef _FE_ASM_BASE

// IR -> ASM outputs this
typedef struct FeAsmBuffer {
    FeAsm** at;
    size_t len;
    size_t cap;

    Arena alloca;

} FeAsmBuffer;

FeAsm* fe_asm_append(FeModule* m, FeAsm* a);
FeAsm* fe_asm(FeModule* m, u8 kind);
FeAsm* fe_asm_inst(FeModule* m, FeArchInstInfo* template);

FeVReg* fe_new_vreg(FeModule* m, u32 regclass);

// fails if no target is provided
void fe_codegen(FeModule* m);


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

enum {
    _FE_ARCH_BEGIN,

    FE_ARCH_APHELION, // aphelion 
    FE_ARCH_XR17032,  // xr/17032
    FE_ARCH_X86_64,   // x86-64
    FE_ARCH_FOX32,    // fox32
    FE_ARCH_ARM64,    // arm64

    _FE_ARCH_END,
};

enum {
    _FE_SYSTEM_BEGIN,

    FE_SYSTEM_NONE,      // freestanding
    
    _FE_SYSTEM_END,
};

enum {
    _FE_PRODUCT_BEGIN,

    FE_PRODUCT_ASSEMBLY, // textual assembly file

    _FE_PRODUCT_END,
};

#include "iron/passes/passes.h"
#include "iron/arch/aphelion/aphelion.h"