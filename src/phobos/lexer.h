#pragma once
#define PHOBOS_LEXER_H

#include "orbit.h"
#include "da.h"

#define TOKEN_LIST \
    TOKEN(tt_invalid, "INVALID") \
    TOKEN(tt_EOF,     "EOF") \
\
    TOKEN(tt_identifier, "identifier") \
    TOKEN(tt_identifier_discard, "_") \
\
    TOKEN(tt_literal_int, "integer literal") \
    TOKEN(tt_literal_float, "float literal") \
    TOKEN(tt_literal_bool, "bool literal") \
    TOKEN(tt_literal_string, "string literal") \
    TOKEN(tt_literal_char, "char literal") \
    TOKEN(tt_literal_null, "null") \
\
    TOKEN(tt_hash,      "#") \
    TOKEN(tt_uninit,    "---") \
    TOKEN(tt_equal,     "=") \
    TOKEN(tt_dollar,    "$") \
    TOKEN(tt_colon,     ":") \
    TOKEN(tt_colon_colon, "::") \
    TOKEN(tt_semicolon, ";") \
    TOKEN(tt_period,    ".") \
    TOKEN(tt_range_less,"..<") \
    TOKEN(tt_range_eq,  "..=") \
    TOKEN(tt_comma,     ",") \
    TOKEN(tt_exclam,    "!") \
    TOKEN(tt_carat,     "^") \
    TOKEN(tt_at,        "@") \
    TOKEN(tt_add,       "+") \
    TOKEN(tt_sub,       "-") \
    TOKEN(tt_mul,       "*") \
    TOKEN(tt_div,       "/") \
    TOKEN(tt_mod,       "%") \
    TOKEN(tt_mod_mod,   "%%") \
    TOKEN(tt_tilde,     "~") \
    TOKEN(tt_and,       "&") \
    TOKEN(tt_or,        "|") \
    TOKEN(tt_nor,       "~|") \
    TOKEN(tt_lshift,    "<<") \
    TOKEN(tt_rshift,    ">>") \
\
    TOKEN(tt_and_and,       "&&") \
    TOKEN(tt_or_or,         "||") \
    TOKEN(tt_tilde_tilde,   "~~") \
\
    TOKEN(tt_arrow_right,   "->") \
    TOKEN(tt_arrow_left,    "<-") \
    TOKEN(tt_arrow_bidir,   "<->") \
\
    TOKEN(tt_add_equal,     "+=") \
    TOKEN(tt_sub_equal,     "-=") \
    TOKEN(tt_mul_equal,     "*=") \
    TOKEN(tt_div_equal,     "/=") \
    TOKEN(tt_mod_equal,     "%=") \
    TOKEN(tt_mod_mod_equal, "%%=") \
\
    TOKEN(tt_and_equal,     "&=") \
    TOKEN(tt_or_equal,      "|=") \
    TOKEN(tt_nor_equal,     "~|=") \
    TOKEN(tt_xor_equal,     "~=") \
    TOKEN(tt_lshift_equal,  "<<=") \
    TOKEN(tt_rshift_equal,  ">>=") \
\
    TOKEN(tt_equal_equal,   "==") \
    TOKEN(tt_not_equal,     "!=") \
    TOKEN(tt_less_than,     "<") \
    TOKEN(tt_less_equal,    "<=") \
    TOKEN(tt_greater_than,  ">") \
    TOKEN(tt_greater_equal, ">=") \
\
    TOKEN(tt_open_paren,    "(") \
    TOKEN(tt_close_paren,   ")") \
    TOKEN(tt_open_brace,    "{") \
    TOKEN(tt_close_brace,   "}") \
    TOKEN(tt_open_bracket,  "[") \
    TOKEN(tt_close_bracket, "]") \
\
    TOKEN(tt_keyword_let,   "let") \
    TOKEN(tt_keyword_mut,   "mut") \
    TOKEN(tt_keyword_def,   "def") \
    TOKEN(tt_keyword_type,  "type") \
\
    TOKEN(tt_keyword_asm,       "asm") \
    TOKEN(tt_keyword_bitcast,   "bitcast") \
    TOKEN(tt_keyword_break,     "break") \
    TOKEN(tt_keyword_case,      "case") \
    TOKEN(tt_keyword_cast,      "cast") \
    TOKEN(tt_keyword_continue,  "continue") \
    TOKEN(tt_keyword_defer,     "defer") \
    TOKEN(tt_keyword_enum,      "enum") \
    TOKEN(tt_keyword_elif,      "elif") \
    TOKEN(tt_keyword_else,      "else") \
    TOKEN(tt_keyword_extern,    "extern") \
    TOKEN(tt_keyword_fallthrough,"fallthrough") \
    TOKEN(tt_keyword_for,       "for") \
    TOKEN(tt_keyword_fn,        "fn") \
    TOKEN(tt_keyword_goto,      "goto") \
    TOKEN(tt_keyword_if,        "if") \
    TOKEN(tt_keyword_in,        "in") \
    TOKEN(tt_keyword_import,    "import") \
    TOKEN(tt_keyword_module,    "module") \
    TOKEN(tt_keyword_return,    "return") \
    TOKEN(tt_keyword_struct,    "struct") \
    TOKEN(tt_keyword_switch,    "switch") \
    TOKEN(tt_keyword_union,     "union") \
    TOKEN(tt_keyword_while,     "while") \
\
    TOKEN(tt_keyword_inline,    "inline") \
    TOKEN(tt_keyword_sizeof,    "sizeof") \
    TOKEN(tt_keyword_alignof,   "alignof") \
    TOKEN(tt_keyword_offsetof,  "offsetof") \
\
    TOKEN(tt_type_keyword_int,  "int") \
    TOKEN(tt_type_keyword_i8,   "i8") \
    TOKEN(tt_type_keyword_i16,  "i16") \
    TOKEN(tt_type_keyword_i32,  "i32") \
    TOKEN(tt_type_keyword_i64,  "i64") \
\
    TOKEN(tt_type_keyword_uint, "uint") \
    TOKEN(tt_type_keyword_u8,   "u8") \
    TOKEN(tt_type_keyword_u16,  "u16") \
    TOKEN(tt_type_keyword_u32,  "u32") \
    TOKEN(tt_type_keyword_u64,  "u64") \
\
    TOKEN(tt_type_keyword_bool, "bool") \
\
    TOKEN(tt_type_keyword_float,"float") \
    TOKEN(tt_type_keyword_f16,  "f16") \
    TOKEN(tt_type_keyword_f32,  "f32") \
    TOKEN(tt_type_keyword_f64,  "f64") \
\
    TOKEN(tt_type_keyword_addr, "addr") \
    TOKEN(tt_meta_COUNT, "") \

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