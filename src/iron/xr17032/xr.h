#ifndef FE_XR_H
#define FE_XR_H

#include <assert.h>
#include "iron/iron.h"

typedef union {
    u32 imm;
    FeSymbol* sym;
} XrInstImm32OrSym;

static inline bool xr_immsym_is_imm(XrInstImm32OrSym* immsym) {
    return immsym->imm & 1;
}
static inline u32 xr_immsym_imm_val(XrInstImm32OrSym* immsym) {
    return (u32)(immsym->imm >> 1);
}
static inline void xr_immsym_set_imm(XrInstImm32OrSym* immsym, u32 imm) {
    immsym->imm = (1 | (imm << 1));
}

typedef enum : u8 {
    XR_SHIFT_LSH = 0, // left shift
    XR_SHIFT_RSH = 1, // logical right shift
    XR_SHIFT_ASH = 2, // arithmetic right shift
    XR_SHIFT_ROR = 3, // rotate right
} XrShiftKind;

typedef struct {
    u16 imm;
    union {    
        u8 small; // u5
        XrShiftKind xsh;
    };
    u8 shamt;   // u5
} XrInstImm;

typedef struct {
    FeBlock* if_true;
    FeBlock* fake_if_false; // "fake" branch target
} XrInstBranch;

typedef struct {
    FeBlock* target;
} XrInstJump;

typedef enum : FeInstKind {
    XR__INVALID = FE__XR_INST_BEGIN,

    // Jumps (XrInstImmOrSym)
    XR_J,   // Jump
    XR_JAL, // Jump And Link
    
    // Branches
    XR_BEQ,     // Branch Equal
    XR_BNE,     // Branch Not Equal
    XR_BLT,     // Branch Less Than
    XR_BLE,     // Branch Less Than or Equal
    XR_BGT,     // Branch Greater Than
    XR_BGE,     // Branch Greater Than or Equal
    XR_BPE,     // Branch Parity Even (lowest bit 0)
    XR_BPO,     // Branch Parity Odd (lowest bit 1)

    // Immediate Operate (XrInstImm)
    XR_ADDI,    // Add Immediate
    XR_SUBI,    // Subtract Immediate
    XR_SLTI,    // Set Less Than Immediate
    XR_SLTI_S,  // Set Less Than Immediate, Signed
    XR_ANDI,    // And Immediate
    XR_XORI,    // Xor Immediate
    XR_ORI,     // Or Immediate
    XR_LUI,     // Load Upper Immediate
    XR_JALR,    // Jump And Link, Register
    XR_ADR,     // Compute Relative Address

    XR_LOAD8_IO,      // Load Byte, Immediate Offset
    XR_LOAD16_IO,     // Load Int, Immediate Offset
    XR_LOAD32_IO,     // Load Long, Immediate Offset
    XR_STORE8_IO,     // Store Byte, Immediate Offset
    XR_STORE16_IO,    // Store Int, Immediate Offset
    XR_STORE32_IO,    // Store Long, Immediate Offset

    XR_LOAD8_RO,      // Load Byte, Register Offset
    XR_LOAD16_RO,     // Load Int, Register Offset
    XR_LOAD32_RO,     // Load Long, Register Offset
    XR_STORE8_RO,     // Store Byte, Register Offset
    XR_STORE16_RO,    // Store Int, Register Offset
    XR_STORE32_RO,    // Store Long, Register Offset

    XR_STORE8_SI,   // Store Byte, Small Immediate
    XR_STORE16_SI,  // Store Int, Small Immediate
    XR_STORE32_SI,  // Store Long, Small Immediate

    // Register Operate
    XR_SHIFT,   // Various Shift by Register Ammount
    XR_ADD,     // Add Register
    XR_SUB,     // Subtract Register
    XR_SLT,     // Set Less Than Register
    XR_SLT_S,   // Set Less Than Register, Signed
    XR_AND,     // And Register
    XR_XOR,     // Xor Register
    XR_OR,      // Or Register
    XR_NOR,     // Nor Register

    XR_MUL,     // Multiply
    XR_DIV,     // Divide
    XR_DIV_S,   // Divide Signed
    XR_MOD,     // Modulo

    XR_LL,      // Load Locked
    XR_SC,      // Store Conditional

    // void
    XR_MB,      // Memory Barrier
    XR_WMB,     // Write Memory Barrier
    XR_BRK,     // Breakpoint
    XR_SYS,     // System Service

    // Privileged Instructions
    XR_MFCR,    // Move From Control Register
    XR_MTCR,    // Move To Control Register
    XR_HLT,     // Halt Until Next Interrupt
    XR_RFE,     // Return From Exception

    // Psuedo-instructions
    XR_P_B,     // Unconditional Relative Branch
    XR_P_RET,   // Return
    XR_P_JR,    // Jump to Register
    XR_P_MOV,   // Move Register
    XR_P_LI,    // Load 16-bit Immediate
    XR_P_LA,    // Load 32-bit Immediate
    XR_P_NOP,   // No Operation
    XR_P_SHI,   // (LSHI/RSHI/ASHI/RORI)
    XR_P_LOAD8_DIRECT,   // Load Byte from 32-bit Address
    XR_P_LOAD16_DIRECT,  // Load Int from 32-bit Address
    XR_P_LOAD32_DIRECT,  // Load Long from 32-bit Address
    XR_P_STORE8_DIRECT,  // Store Byte to 32-bit Address
    XR_P_STORE16_DIRECT, // Store Int to 32-bit Address
    XR_P_STORE32_DIRECT, // Store Long to 32-bit Address

    // fake instruction for obtaining a temp register for a STORE*_DIRECT
    // skill issue for my register allocator lmfao
    XR__STORE_DIRECT_SYMBOL,
} XrInstKind;

typedef enum : FeRegClass {
    XR_REGCLASS_GPR,
    XR_REGCLASS__COUNT,
} XrRegClasses;

typedef enum : u16 {
    XR_GPR_ZERO = 0,

    // temps (call-clobbered)
    XR_GPR_T0, XR_GPR_T1, XR_GPR_T2, XR_GPR_T3, XR_GPR_T4, XR_GPR_T5,
    
    // args (call-clobbered)
    XR_GPR_A0, XR_GPR_A1, XR_GPR_A2, XR_GPR_A3,
    
    // local vars (call-preserved)
    XR_GPR_S0, XR_GPR_S1, XR_GPR_S2, XR_GPR_S3, XR_GPR_S4,
    XR_GPR_S5, XR_GPR_S6, XR_GPR_S7, XR_GPR_S8, XR_GPR_S9,
    XR_GPR_S10, XR_GPR_S11, XR_GPR_S12, XR_GPR_S13, XR_GPR_S14,
    XR_GPR_S15, XR_GPR_S16, XR_GPR_S17,

    // thread pointer (dont touch!)
    XR_GPR_TP,
    
    // stack pointer (sometimes touch!)
    XR_GPR_SP,
    
    // link register (very only sometimes touch!)
    XR_GPR_LR,

    XR_GPR__COUNT,
} XrGpr;

FeInstChain fe_xr_isel(FeFunc* f, FeBlock* block, FeInst* inst);
void fe_xr_post_regalloc_reduce(FeFunc* f);
FeRegClass fe_xr_choose_regclass(FeInstKind kind, FeTy ty);
void fe_xr_print_inst(FeDataBuffer* db, FeFunc* f, FeInst* inst);

const char* fe_xr_reg_name(u8 regclass, u16 real);
FeRegStatus fe_xr_reg_status(u8 cconv, u8 regclass, u16 real);

extern const u8 fe_xr_extra_size_table[];
extern const FeTrait fe_xr_trait_table[];
extern const u16 fe_xr_regclass_lens[];

#endif
