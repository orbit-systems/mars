#pragma once

#include "iron/iron.h"

enum {
    TOK_EOF,

    TOK_OPEN_PAREN,
    TOK_CLOSE_PAREN,

    TOK_IDENT,
    TOK_STRING,
    TOK_NUMERIC,
};

typedef struct Token {
    u64 index : 46;
    u64 len : 15;
    u64 type : 3;
} Token;

da_typedef(Token);

static_assert(sizeof(Token) == sizeof(u64));

typedef struct Lexer {
    string src;
    u64 index;
    char current;
} Lexer;

enum {
    AST_NULL,
    AST_NUMERIC,
    AST_STRING,
    AST_IDENT,
    AST_LIST,
};

typedef struct Ast {
    u64 token_index : 56;
    u64 kind : 8;
} Ast;

typedef struct AstNumeric {
    Ast base;
    u64 value;
} AstNumeric;

typedef struct AstString {
    Ast base;
    string value;
} AstString;

typedef struct AstIdent {
    Ast base;
} AstIdent;

typedef struct AstList {
    Ast base;
    Ast* this;
    Ast* next;
} AstList;

typedef struct Parser {
    Arena node_alloca;

    Token* tokens;
    u64 tokens_len;
    u64 index;
    Token current;

    string text;
} Parser;

Ast* parse(Parser* p);