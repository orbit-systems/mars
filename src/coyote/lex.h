#ifndef LEX_H
#define LEX_H

#include "coyote.h"

#define _LEX_KEYWORDS_ \
    T(AND) \
    T(OR) \
    T(BREAK) \
    T(BYTE) \
    T(CAST) \
    T(CONTAINEROF) \
    T(CONTINUE) \
    T(DO) \
    T(ELSE) \
    T(ELSEIF) \
    T(END) \
    T(ENUM) \
    T(FALSE) \
    T(FN) \
    T(FNPTR) \
    T(GOTO) \
    T(IF) \
    T(IN) \
    T(INT) \
    T(LEAVE) \
    T(LONG) \
    T(NOT) \
    T(NOTHING) \
    T(NULLPTR) \
    T(OUT) \
    T(PACKED) \
    T(RETURN) \
    T(ROR) \
    T(SIZEOF) \
    T(OFFSETOF) \
    T(SIZEOFVALUE) \
    T(STRUCT) \
    T(THEN) \
    T(TO) \
    T(TRUE) \
    T(TYPE) \
    T(UBYTE) \
    T(UINT) \
    T(ULONG) \
    T(UNION) \
    T(VOID) \
    T(WHILE) \
    T(BARRIER) \
    T(INSERTASM) \
    \
    T(PUBLIC) \
    T(PRIVATE) \
    T(EXPORT) \
    T(EXTERN) \
    \
    T(UQUAD) \
    T(QUAD) \
    T(UWORD) \
    T(WORD) \
    \
    T(JKL_FILE_NAME) \
    T(JKL_FUNC_NAME) \
    T(JKL_LINE_NUMBER) \
    \
    /* coyote extensions begin here */ \
    \
    T(NOALIAS) \
    T(NORETURN) \
    T(ALIGNOF) \
    T(ALIGNOFVALUE) \
    T(UNREACHABLE) \

typedef struct {
    string src;
    string path;
} SrcFile;

typedef struct {
    string src;
    usize cursor;
    char current;
    bool eof;
    Arena* arena;
} Lexer;

#define LEX_MAX_TOKEN_LEN 127

#ifndef __x86_64__
    #error "this trick only works on x86-64 lmao"
#endif
typedef struct {
    u64 kind : 7;
    // was this token "generated" by the preprocessor?
    // this means that the span might not correspond to source text.
    u64 generated : 1;
    // textual content
    u64 len : 8;
    i64 raw : 48;
} Token;
static_assert(sizeof(Token) == 8);

#define tok_raw(t) ((char*)(i64)((t).raw))

enum {
    TOK__INVALID,
    TOK_EOF,

    TOK_OPEN_PAREN,    // (
    TOK_CLOSE_PAREN,   // )
    TOK_OPEN_BRACE,    // {
    TOK_CLOSE_BRACE,   // }
    TOK_OPEN_BRACKET,  // [
    TOK_CLOSE_BRACKET, // ]

    TOK_COLON,      // :
    TOK_COMMA,      // ,
    TOK_DOT,        // .
    TOK_AT,         // @
    TOK_TILDE,      // ~
    TOK_CARET,      // ^
    TOK_CARET_DOT,  // ^.  very common pattern

    TOK_VARARG,     // ...

    TOK_EQ,         // =
    TOK_PLUS_EQ,    // +=
    TOK_MINUS_EQ,   // -=
    TOK_MUL_EQ,     // *=
    TOK_DIV_EQ,     // /=
    TOK_REM_EQ,     // %=
    TOK_AND_EQ,     // &=
    TOK_OR_EQ,      // |=
    TOK_XOR_EQ,     // $=
    TOK_LSHIFT_EQ,  // <<=
    TOK_RSHIFT_EQ,  // >>=

    TOK_PLUS,       // +
    TOK_MINUS,      // -
    TOK_MUL,        // *
    TOK_DIV,        // /
    TOK_REM,        // %
    TOK_AND,        // &
    TOK_OR,         // |
    TOK_XOR,        // $
    TOK_LSHIFT,     // <<
    TOK_RSHIFT,     // >>

    TOK_EQ_EQ,      // ==
    TOK_NOT_EQ,     // !=
    TOK_LESS_EQ,    // <=
    TOK_GREATER_EQ, // >=
    TOK_LESS,       // <
    TOK_GREATER,    // >

    TOK_IDENTIFIER,

    TOK_CHAR,
    TOK_STRING,
    TOK_INTEGER,

    // only for internal lexer use
    // doesn't appear in final token stream
    TOK_HASH,

    TOK__KEYWORDS_BEGIN,

    #define T(ident) TOK_KW_##ident,
        _LEX_KEYWORDS_
    #undef T

    TOK__KEYWORDS_END,

    // these get preserved and passed onto the parser
    TOK_PREPROC_SECTION,
    TOK_PREPROC_ENTERSECTION,
    TOK_PREPROC_LEAVESECTION,
    TOK_PREPROC_ASM,

    // 'invisible' tokens, mostly for tracking
    // preprocessor stuff so error messages can 
    // trace it, and the parser can correctly 
    // scope things later
    TOK__PARSE_IGNORE,

        TOK_PREPROC_MACRO_PASTE, // before a macro is invoked in source code
        // TOK_PREPROC_MACRO_ARG_PASTE, // before an argument to a macro gets replaced in the macro's body
        TOK_PREPROC_DEFINE_PASTE, // before a define's replacement gets pasted
        TOK_PREPROC_INCLUDE_PASTE, // before a file is included
        TOK_PREPROC_PASTE_END, // marks the end of a paste action

        TOK_NEWLINE, 

    TOK__PARSE_IGNORE_END,

    TOK__COUNT
};
static_assert(TOK__COUNT < (1 << 8));

extern const char* token_kind[TOK__COUNT];

VecPtr_typedef(SrcFile);
typedef struct ParseScope ParseScope;
typedef struct ParseScope {
    StrMap map;
    ParseScope* super;
    ParseScope* sub;
} ParseScope;

typedef struct Entity Entity;

typedef struct FlagSet {
    bool xrsdk: 1;
    bool error_on_warn: 1;
    bool preproc: 1;
} FlagSet;

typedef struct {
    Token current;
    Token* tokens;
    u32 tokens_len;
    u32 cursor;

    ParseScope* global_scope;
    ParseScope* current_scope;

    VecPtr(SrcFile) sources;

    Entity* current_function;

    struct Stmt* recent_while;

    Arena arena;
    Arena entities;

    FlagSet flags;
} Parser;

Vec_typedef(Token);

Parser lex_entrypoint(SrcFile* f);
string tok_span(Token t);

#define MAX_MACRO_ARGS 255

#ifdef __x86_64__
    typedef struct {
        u64 len: 16;
        i64 raw: 48;
    } CompactString;
    static_assert(sizeof(CompactString) == 8);

    #define COMPACT_STR_MAX_LEN UINT16_MAX
    #define to_compact(str) (CompactString){.len = (str).len, .raw = (i64)(str).raw}
    #define from_compact(str) (string){.len = (str).len, .raw = (char*)(i64)(str).raw}
#else
    typedef string CompactString;
    #define COMPACT_STR_MAX_LEN UINT64_MAX
    #define to_compact(str) (str)
    #define from_compact(str) (str)
#endif

enum {
    PPVAL_NONE,
    
    PPVAL_STRING,
    PPVAL_INTEGER,
    PPVAL_COMPLEX_STRING,

    PPVAL_MACRO,
};

typedef struct PreprocScope {
    StrMap map;
    struct PreprocScope* parent;
} PreprocScope;

typedef struct {
    u64 is_macro_arg : 1;
    u64 kind : 7;
    u64 len : 8;
    i64 raw : 48; // non-zero if can be reconstructed from a token.
    CompactString source; // where this thing comes from
    union {
        CompactString string;
        i64 integer;
        struct {
            u64 params_index: 24;
            u64 params_len : 8;
            u64 body_index : 32;
        } macro;
    };
} PreprocVal;

Vec_typedef(PreprocVal);

typedef enum : u8 {
    REPORT_ERROR,
    REPORT_WARNING,
    REPORT_NOTE,
} ReportKind;

typedef struct {
    ReportKind kind;
    string src; 
    string path; 
    string snippet; 
    string msg;

    string reconstructed_line;
    string reconstructed_snippet;
} ReportLine;

void report_line(ReportLine* line);

#endif // LEX_H
