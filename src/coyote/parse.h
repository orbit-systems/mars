#ifndef PARSE_H
#define PARSE_H

#include "common/vec.h"
#include "coyote.h"
#include "lex.h"

// ----------------- TYPES AND SHIT ----------------- 

typedef enum : u8 {
    TY__INVALID,

    TY_VOID,

    TY_BYTE,    // i8
    TY_UBYTE,   // u8
    TY_INT,     // i16
    TY_UINT,    // u16
    TY_LONG,    // i32
    TY_ULONG,   // u32
    TY_QUAD,    // i64
    TY_UQUAD,   // u64

    TY_PTR,
    TY_ARRAY,
    TY_STRUCT,
    TY_STRUCT_PACKED,
    TY_UNION,
    TY_ENUM,
    TY_FN,

    TY_ALIAS_INCOMPLETE,
    // TY_ALIAS_IN_PROGRESS,
    TY_ALIAS,
} TyKind;

typedef u16 TyIndex;
typedef struct {u32 _;} TyBufSlot;

typedef struct {
    TyKind kind;
} TyBase;

typedef struct {
    u8 kind;
    TyIndex to;
} TyPtr;

typedef struct {
    u8 kind;
    TyIndex to;
    u32 len;
} TyArray;

typedef struct {
    u8 kind;
    TyIndex backing_ty;
} TyEnum;

typedef struct {
    u8 kind;
    TyIndex aliasing;
    Entity* entity;
} TyAlias;

typedef struct {
    TyIndex type;
    u16 offset;
    CompactString name;
} Ty_RecordMember;

#define TY_RECORD_MAX_MEMBERS UINT8_MAX
typedef struct {
    u8 kind;
    u8 len; // should never be a limitation O.O
    u16 size;
    u8 align;
    Ty_RecordMember members[];
} TyRecord;

typedef union {
    struct {    
        TyIndex ty;
        bool out;
        CompactString name;
    };
    struct {
        CompactString argv;
        CompactString argc;
    } varargs;
} Ty_FnParam;

#define TY_FN_MAX_PARAMS 32 // reasonable
typedef struct {
    u8 kind;
    u8 len;
    bool variadic; // last parameter is a variadic argument
    bool is_noreturn;
    TyIndex ret_ty;
    CompactString name;
    Ty_FnParam params[];
} TyFn;

void ty_init();

// ------------------- PARSE/SEMA ------------------- 

void token_error(Parser* ctx, ReportKind kind, u32 start_index, u32 end_index, const char* msg);

typedef struct Expr Expr;
typedef struct Stmt Stmt;

typedef enum : u8 {
    STORAGE_LOCAL,
    STORAGE_OUT_PARAM,
    STORAGE_PUBLIC,
    STORAGE_PRIVATE,
    STORAGE_EXPORT,
    STORAGE_EXTERN,
} StorageKind;

typedef enum : u8 {
    ENTKIND_VAR = 1,
    ENTKIND_FN,
    ENTKIND_TYPE,
    ENTKIND_LABEL,
    ENTKIND_VARIANT, // enum variant, treated like a fucked up constant
} EntityKind;

typedef struct Entity {
    CompactString name;

    EntityKind kind;
    StorageKind storage;
    TyIndex ty;

    union {
        Stmt* decl;
        u64 variant_value;
    };
} Entity;

typedef enum : u8 {
    
    STMT_EXPR,

    STMT_DECL_LOCATION,

    STMT_VAR_DECL,
    STMT_FN_DECL,

    STMT_ASSIGN,
    STMT_ASSIGN_ADD,
    STMT_ASSIGN_SUB,
    STMT_ASSIGN_MUL,
    STMT_ASSIGN_DIV,
    STMT_ASSIGN_MOD,
    STMT_ASSIGN_AND,
    STMT_ASSIGN_OR,
    STMT_ASSIGN_XOR,
    STMT_ASSIGN_LSH,
    STMT_ASSIGN_RSH,

    STMT_BARRIER,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_LEAVE,
    STMT_RETURN,

    STMT_IF,
    STMT_BLOCK,

    STMT_WHILE,

    STMT_LABEL,
    STMT_GOTO,

    STMT_UNREACHABLE,
} StmtKind;

typedef enum : u8 {
    RETKIND_NO,     // does not return
    RETKIND_MAYBE,  // might possibly return
    RETKIND_YES,    // will return
} ReturnKind;

typedef struct StmtList {
    u32 len;
    ReturnKind retkind;
    Stmt** stmts;
} StmtList;

typedef struct Stmt {
    StmtKind kind;
    ReturnKind retkind;
    u32 token_index;
    
    union { // allocated in the arena as-needed
        usize extra[0];

        usize nothing[0];
        
        Expr* expr;

        struct {
            Entity* var;
            Expr* expr;
        } var_decl;

        struct {
            Entity* fn;
            StmtList body;
        } fn_decl;

        struct {
            Expr* lhs;
            Expr* rhs;
        } assign;

        struct {
            Expr* cond;
            StmtList block;
            Stmt* else_;
        } if_;

        StmtList block;

        struct {
            Expr* cond;
            StmtList block;
        } while_;

        Entity* label;

        Entity* goto_;
    };
} Stmt;

typedef enum : u8 {
    EXPR_ADD,
    EXPR_SUB,
    EXPR_MUL,
    EXPR_DIV,
    EXPR_REM,
    EXPR_AND,
    EXPR_OR,
    EXPR_XOR,
    EXPR_LSH,
    EXPR_RSH,
    EXPR_ROR,
    
    EXPR_BOOL_OR,
    EXPR_BOOL_AND,

    EXPR_EQ,
    EXPR_NEQ,
    EXPR_LESS_EQ,
    EXPR_GREATER_EQ,
    EXPR_LESS,
    EXPR_GREATER,

    EXPR_ADDROF,
    EXPR_NEG,
    EXPR_NOT,
    EXPR_BOOL_NOT,
    EXPR_SIZEOFVALUE,
    EXPR_OUT_ARG,

    EXPR_CONTAINEROF,
    EXPR_CAST,

    EXPR_ENTITY,
    EXPR_STR_LITERAL,
    EXPR_LITERAL,
    EXPR_COMPOUND_LITERAL,
    EXPR_EMPTY_COMPOUND_LITERAL,

    EXPR_INDEXED_ITEM,

    EXPR_PTR_INDEX,     // foo[bar]
    EXPR_ARRAY_INDEX,   // foo[bar]
    EXPR_DEREF,         // foo^
    EXPR_DEREF_MEMBER,  // foo^.bar
    EXPR_MEMBER,        // foo.bar
    EXPR_CALL,          // foo(bar, baz)
} ExprKind;

typedef struct Expr {
    ExprKind kind;
    TyIndex ty;
    u32 token_index;
    
    union { // allocated in the arena as-needed
        usize extra[0];

        usize nothing[0];

        u64 literal;
        CompactString lit_string;

        struct {
            Expr* value;
            u32 index;
        } indexed_item;

        struct {
            Expr** values;
            u32 len;
        } compound_lit;

        Entity* entity;

        Expr* unary;

        struct {
            Expr* lhs;
            Expr* rhs;
        } binary;

        struct {
            Expr* callee;
            Expr** args;
            u32 args_len;
        } call;

        struct {
            Expr* aggregate;
            u32 member_index;
        } member_access;
    };
} Expr;

Stmt* parse_stmt(Parser* p);
Expr* parse_expr(Parser* p);
TyIndex parse_type(Parser* p, bool allow_incomplete);

VecPtr_typedef(Entity);

typedef struct CompilationUnit {
    Arena arena;
    ParseScope* top_scope;

    Token* tokens;
    u32 tokens_len;

    VecPtr(SrcFile) sources;
} CompilationUnit;

CompilationUnit parse_unit(Parser* p);

#endif // PARSE_H
