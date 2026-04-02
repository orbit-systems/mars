#ifndef MARS_PARSE_H
#define MARS_PARSE_H

#include "lex.h"
#include <stddef.h>

typedef struct ParseExpr ParseExpr;

typedef enum: u8 {
    EXPR__INVALID = 0,

    EXPR_IDENT,                 // x
    EXPR_UNDEFINED,             // undefined
    EXPR_NUMERIC,               // 123, 123.0
    EXPR_STRING,                // "hello!"
    EXPR_NULL,                  // null
    EXPR_TRUE,                  // true
    EXPR_FALSE,                 // false
    EXPR_SELF,                  // self
    EXPR_BUILTIN,               // builtin

    EXPR_TY_ISIZE,              // isize
    EXPR_TY_USIZE,              // usize
    EXPR_TY_INT,                // i0 ... i64
    EXPR_TY_UINT,               // u0 ... u64
    EXPR_TY_FLOAT,              // f16, f32, f64, etc.
    EXPR_TY_TYPE,               // type
    EXPR_TY_VOID,               // void
    EXPR_TY_NORETURN,           // noreturn
    EXPR_TY_SELF,               // Self

    EXPR_TY_MODULE,             // module {...}
    EXPR_TY_STRUCT,             // struct {...}
    EXPR_TY_UNION,              // union {...}
    EXPR_TY_ENUM,               // enum {...}
    EXPR_TY_POINTER,            // ^T
    EXPR_TY_SLICE,              // []T
    EXPR_TY_SLICE_POINTER,      // [^]T
    EXPR_TY_ARRAY,              // [N]T
    EXPR_TY_VECTOR,             // vector[N]T
    EXPR_TY_OPTIONAL,           // ?T
    EXPR_TY_ENUM_OPTIONAL,      // E?T

    EXPR_ENUM_ANON,             // enum

    EXPR_NEG,                   // -x
    EXPR_ADDR,                  // &x
    EXPR_BIT_NOT,               // ~x
    EXPR_BOOL_NOT,              // !x

    EXPR_BOOL_AND,              // x && y
    EXPR_BOOL_OR,               // x || y

    EXPR_BIT_AND,               // x & y
    EXPR_BIT_OR,                // x | y
    EXPR_BIT_XOR,               // x ~ y

    EXPR_ADD,                   // x + y
    EXPR_SUB,                   // x - y
    EXPR_MUL,                   // x * y
    EXPR_DIV,                   // x / y
    EXPR_REM,                   // x % y

    EXPR_BREAK,                 // break
    EXPR_RETURN,                // return (x, y)
    EXPR_UNREACHABLE,           // unreachable

    EXPR_SELECTOR_ANON,         // .member
    EXPR_NAMESPACE_ANON,        // ::member
    EXPR_SELECTOR,              // x.member
    EXPR_NAMESPACE,             // x::member

    EXPR_SELECTOR_ENUM,         // x.enum
    EXPR_NAMESPACE_ENUM,        // x::enum

    EXPR_PAREN,                 // (x)
    EXPR_TUPLE_LIT,             // (x, y, z)
    EXPR_COMPOUND_LIT,          // T.{x = x, y = y, z = z}
    EXPR_COMPOUND_LIT_ANON,     // .{x = x, y = y, z = z}

    EXPR_BLOCK,                 // {...}
    EXPR_IF_BLOCK,              // if cond {}
    EXPR_IF_THEN,               // if cond then y
    EXPR_ELSE,                  // x else y
    EXPR_ELSE_BLOCK,            // x else {}
    EXPR_SWITCH,                // switch x
    EXPR_CASE_BLOCK,            // case x {}
    EXPR_CASE_THEN,             // case x then y
    EXPR_TRY,                   // try x
    EXPR_TRY_BLOCK,             // try: {}
    EXPR_LABEL,                 // foo: {}

    EXPR_QUAL_OPT_NOINLINE,     // 'noinline' optional qualifier

    EXPR_QUAL_STR_ALIGNAS,      // 'alignas' struct qualifier
    EXPR_QUAL_STR_PACKED,       // 'packed' struct qualifier
    EXPR_QUAL_STR_BITPACKED,    // 'bitpacked' struct qualifier

    EXPR_QUAL_REF_CONST,        // 'const' reference qualifier
    EXPR_QUAL_REF_STATIC,       // 'static' reference qualifier
    EXPR_QUAL_REF_NOALIAS,      // 'noalias' reference qualifier
    EXPR_QUAL_REF_ALLOWZERO,    // 'allowzero' reference qualifier
    EXPR_QUAL_REF_ALIGNAS,      // 'alignas' reference qualifier
    EXPR_QUAL_REF_OFFSETOF,     // 'offsetof' reference qualifier

    EXPR_FUN_SIGNATURE,

    EXPR_BAD,                   // parse failure
} ParseExprKind;

struct ParseExpr {
    ParseExprKind kind;
    u32 leftmost_token;
    union {
        void* nothing[0];
        ParseExpr* un;
        struct {
            ParseExpr* lhs;
            ParseExpr* rhs;
        } bin;
    };
};

#define FAKE_PTR(T) ((T*)nullptr)
#define PEXPR_SIZE(field) (offsetof(ParseExpr, nothing) + sizeof(FAKE_PTR(ParseExpr)->field))

typedef enum: u8 {
    STMT__INVALID = 0,

    STMT_MODULE,
    STMT_LIBRARY,

    STMT_EXPR,

    STMT_ASSIGN,
    STMT_ASSIGN_ADD,

    STMT_FUN_DECL,

    STMT_DEFER,

    STMT_IMPL_BLOCK,
} ParseStmtKind;

typedef struct ParseStmt {
    ParseStmtKind kind;
    //...
} ParseStmt;

typedef struct ParseTree {
    Arena arena;

    // ParseStmt* top;
} ParseTree;

#endif // MARS_PARSE_H
