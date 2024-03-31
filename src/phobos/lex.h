#pragma once
#define PHOBOS_LEXER_H

#include "orbit.h"


#define TOKEN_LIST \
    TOKEN(TT_INVALID, "INVALID") \
    TOKEN(TT_EOF,     "EOF") \
\
    TOKEN(TT_IDENTIFIER, "identifier") \
    TOKEN(TT_IDENTIFIER_DISCARD, "_") \
\
    TOKEN(TT_LITERAL_INT, "integer literal") \
    TOKEN(TT_LITERAL_FLOAT, "float literal") \
    TOKEN(TT_LITERAL_BOOL, "bool literal") \
    TOKEN(TT_LITERAL_STRING, "string literal") \
    TOKEN(TT_LITERAL_CHAR, "char literal") \
    TOKEN(TT_LITERAL_NULL, "null") \
\
    TOKEN(TT_HASH,      "#") \
    TOKEN(TT_UNINIT,    "---") \
    TOKEN(TT_EQUAL,     "=") \
    TOKEN(TT_DOLLAR,    "$") \
    TOKEN(TT_COLON,     ":") \
    TOKEN(TT_COLON_COLON, "::") \
    TOKEN(TT_SEMICOLON, ";") \
    TOKEN(TT_PERIOD,    ".") \
    TOKEN(TT_RANGE_LESS,"..<") \
    TOKEN(TT_RANGE_EQ,  "..=") \
    TOKEN(TT_COMMA,     ",") \
    TOKEN(TT_EXCLAM,    "!") \
    TOKEN(TT_CARAT,     "^") \
    TOKEN(TT_AT,        "@") \
    TOKEN(TT_ADD,       "+") \
    TOKEN(TT_SUB,       "-") \
    TOKEN(TT_MUL,       "*") \
    TOKEN(TT_DIV,       "/") \
    TOKEN(TT_MOD,       "%") \
    TOKEN(TT_MOD_MOD,   "%%") \
    TOKEN(TT_TILDE,     "~") \
    TOKEN(TT_AND,       "&") \
    TOKEN(TT_OR,        "|") \
    TOKEN(TT_NOR,       "~|") \
    TOKEN(TT_LSHIFT,    "<<") \
    TOKEN(TT_RSHIFT,    ">>") \
\
    TOKEN(TT_AND_AND,       "&&") \
    TOKEN(TT_OR_OR,         "||") \
 /* TOKEN(TT_TILDE_TILDE, "~~") */ \
\
    TOKEN(TT_ARROW_RIGHT,   "->") \
    TOKEN(TT_ARROW_LEFT,    "<-") \
    TOKEN(TT_ARROW_BIDIR,   "<->") \
\
    TOKEN(TT_ADD_EQUAL,     "+=") \
    TOKEN(TT_SUB_EQUAL,     "-=") \
    TOKEN(TT_MUL_EQUAL,     "*=") \
    TOKEN(TT_DIV_EQUAL,     "/=") \
    TOKEN(TT_MOD_EQUAL,     "%=") \
    TOKEN(TT_MOD_MOD_EQUAL, "%%=") \
\
    TOKEN(TT_AND_EQUAL,     "&=") \
    TOKEN(TT_OR_EQUAL,      "|=") \
    TOKEN(TT_NOR_EQUAL,     "~|=") \
    TOKEN(TT_XOR_EQUAL,     "~=") \
    TOKEN(TT_LSHIFT_EQUAL,  "<<=") \
    TOKEN(TT_RSHIFT_EQUAL,  ">>=") \
\
    TOKEN(TT_EQUAL_EQUAL,   "==") \
    TOKEN(TT_NOT_EQUAL,     "!=") \
    TOKEN(TT_LESS_THAN,     "<") \
    TOKEN(TT_LESS_EQUAL,    "<=") \
    TOKEN(TT_GREATER_THAN,  ">") \
    TOKEN(TT_GREATER_EQUAL, ">=") \
\
    TOKEN(TT_OPEN_PAREN,    "(") \
    TOKEN(TT_CLOSE_PAREN,   ")") \
    TOKEN(TT_OPEN_BRACE,    "{") \
    TOKEN(TT_CLOSE_BRACE,   "}") \
    TOKEN(TT_OPEN_BRACKET,  "[") \
    TOKEN(TT_CLOSE_BRACKET, "]") \
\
    TOKEN(TT_KEYWORD_LET,   "let") \
    TOKEN(TT_KEYWORD_MUT,   "mut") \
    TOKEN(TT_KEYWORD_DEF,   "def") \
    TOKEN(TT_KEYWORD_TYPE,  "type") \
\
    TOKEN(TT_KEYWORD_ASM,       "asm") \
    TOKEN(TT_KEYWORD_BITCAST,   "bitcast") \
    TOKEN(TT_KEYWORD_BREAK,     "break") \
    TOKEN(TT_KEYWORD_CASE,      "case") \
    TOKEN(TT_KEYWORD_CAST,      "cast") \
    TOKEN(TT_KEYWORD_CONTINUE,  "continue") \
    TOKEN(TT_KEYWORD_DEFER,     "defer") \
    TOKEN(TT_KEYWORD_DISTINCT,  "distinct") \
    TOKEN(TT_KEYWORD_ENUM,      "enum") \
    TOKEN(TT_KEYWORD_ELIF,      "elif") \
    TOKEN(TT_KEYWORD_ELSE,      "else") \
    TOKEN(TT_KEYWORD_EXTERN,    "extern") \
    TOKEN(TT_KEYWORD_FALLTHROUGH,"fallthrough") \
    TOKEN(TT_KEYWORD_FOR,       "for") \
    TOKEN(TT_KEYWORD_FN,        "fn") \
    TOKEN(TT_KEYWORD_IF,        "if") \
    TOKEN(TT_KEYWORD_IN,        "in") \
    TOKEN(TT_KEYWORD_IMPORT,    "import") \
    TOKEN(TT_KEYWORD_MODULE,    "module") \
    TOKEN(TT_KEYWORD_RETURN,    "return") \
    TOKEN(TT_KEYWORD_STRUCT,    "struct") \
    TOKEN(TT_KEYWORD_SWITCH,    "switch") \
    TOKEN(TT_KEYWORD_UNION,     "union") \
    TOKEN(TT_KEYWORD_WHILE,     "while") \
\
    TOKEN(TT_KEYWORD_INLINE,    "inline") \
    TOKEN(TT_KEYWORD_SIZEOF,    "sizeof") \
    TOKEN(TT_KEYWORD_ALIGNOF,   "alignof") \
    TOKEN(TT_KEYWORD_OFFSETOF,  "offsetof") \
\
    TOKEN(TT_TYPE_KEYWORD_INT,  "int") \
    TOKEN(TT_TYPE_KEYWORD_I8,   "i8") \
    TOKEN(TT_TYPE_KEYWORD_I16,  "i16") \
    TOKEN(TT_TYPE_KEYWORD_I32,  "i32") \
    TOKEN(TT_TYPE_KEYWORD_I64,  "i64") \
\
    TOKEN(TT_TYPE_KEYWORD_UINT, "uint") \
    TOKEN(TT_TYPE_KEYWORD_U8,   "u8") \
    TOKEN(TT_TYPE_KEYWORD_U16,  "u16") \
    TOKEN(TT_TYPE_KEYWORD_U32,  "u32") \
    TOKEN(TT_TYPE_KEYWORD_U64,  "u64") \
\
    TOKEN(TT_TYPE_KEYWORD_BOOL, "bool") \
\
    TOKEN(TT_TYPE_KEYWORD_FLOAT,"float") \
    TOKEN(TT_TYPE_KEYWORD_F16,  "f16") \
    TOKEN(TT_TYPE_KEYWORD_F32,  "f32") \
    TOKEN(TT_TYPE_KEYWORD_F64,  "f64") \
\
    TOKEN(TT_TYPE_KEYWORD_ADDR, "addr") \
    TOKEN(TT_META_COUNT, "") \

typedef u8 token_type; enum {
#define TOKEN(enum, str) enum,
TOKEN_LIST
#undef TOKEN
};

extern char* token_type_str[];

typedef struct token_s {
    string text;
    token_type type;
} token;

da_typedef(token);

typedef struct lexer_s {
    string src;
    string path;
    da(token) buffer;
    u64 cursor;
    char current_char;
} lexer;

lexer new_lexer(string path, string src);
void construct_token_buffer(lexer* lex);
void append_next_token(lexer* lex);