#include "iron/iron.h"

// read IR from its textual representation.

enum {
    TOK_EOF,

    TOK_OPEN_PAREN,
    TOK_CLOSE_PAREN,

    TOK_IDENT,
    TOK_STRING,
    TOK_NUMERIC,
};

typedef struct IrToken {
    u64 index : 50;
    u64 len : 10;
    u64 type : 4;
} IrToken;
static_assert(sizeof(IrToken) == sizeof(u64));

typedef struct IrLexer {
    string src;
    u64 index;
    char current;
} IrLexer;

static forceinline void advance(IrLexer* l) {
    if (l->index <= l->src.len) {
        l->index++;
        l->current = l->src.raw[l->index];
    } 
}

static forceinline bool is_eof(IrLexer* l) {
    return l->index > l->src.len;
}

static forceinline IrToken make_eof(IrLexer* l) {
    return (IrToken){l->index, 0, TOK_EOF};
}

static forceinline bool is_whitespace(IrLexer* l) {
    switch (l->current) {
    case '\t':
    case '\v':
    case '\r':
    case '\n':
    case ' ':
        return true;
    default:
        return false;
    }
}

static forceinline bool is_numeric(IrLexer* l) {
    return '0' <= l->current && l->current <= '9';
}

static forceinline bool is_special(IrLexer* l) {
    return is_whitespace(l) || l->current == '(' || l->current == ')' || l->current == '"';
}

static IrToken next_token(IrLexer* l) {
    while (is_whitespace(l)) advance(l);

    if (is_eof(l)) return make_eof(l);

    IrToken t;
    switch (l->current) {
    case '(':
        t.index = l->index;
        t.len = 1;
        t.type = TOK_OPEN_PAREN;
        advance(l);
        break;
    case ')':
        t.index = l->index;
        t.len = 1;
        t.type = TOK_CLOSE_PAREN;
        advance(l);
        break;
    case '"':
        t.index = l->index;
        t.type = TOK_STRING;
        advance(l);
        while (!is_eof(l) && l->current != '"') {
            advance(l);
        }
        advance(l);
        t.len = l->index - t.index;
        break;
    default:
        t.index = l->index;
        t.type = TOK_NUMERIC;
        while (!is_eof(l) && !is_special(l)) {
            if (!is_numeric(l)) t.type = TOK_IDENT;
            advance(l);
        }
        t.len = l->index - t.index;
        break;
    }

    return t;
}

enum {
    AST_LIST = 1,
    AST_STRING,
    AST_NUMERIC,
    AST_IDENT,
};

typedef struct IrAst IrAst;
typedef struct IrAst {
    u32 token_index;
    u8 kind;
    union {
        string string;
        u64 numeric;
        string ident;
        struct {
            IrAst* this;
            IrAst* next;
        } list;
    };
} IrAst;
da_typedef(IrToken);

typedef struct IrParser {
    Arena alloca;
    Arena string_alloca;
    IrToken* tokens;
    u64 tokens_len;
    u64 index;
    string src;
} IrParser;

static forceinline IrAst* new_ast(IrParser* p, u8 kind) {
    IrAst* ast = arena_alloc(&p->alloca, sizeof(IrAst), alignof(IrAst));
    ast->kind = kind;
    return ast;
}

static forceinline IrToken current_token(IrParser* p) {
    return p->tokens[p->index];
}

static forceinline void advance_token(IrParser* p) {
    p->index++;
}

static forceinline string token_to_string(string src, IrToken t) {
    return (string){
        .len = t.len,
        .raw = &src.raw[t.index]
    };
}

static u64 parse_integer(string n) {
    u64 num = 0;
    for_range(i, 0, n.len) {
        u64 charval = n.raw[i] - '0';
        num = num*10 + charval;
    }
    return num;
}

static forceinline u64 char_to_digit(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    CRASH("invalid digit %c", c);
}

static string parse_string(IrParser* p, string s) {
    assert(s.raw[0] == '"' && s.raw[s.len-1] == '"');
    s.raw += 1;
    s.len -= 2; // trim off leading and trailing "

    u64 len = 0;
    for_urange(i, 0, s.len) {
        char c = s.raw[i];
        if (c == '\\') {
            if (s.len < i + 2) CRASH("not enough room for escape sequence");
            i += 2;
        }
        len += 1;
    }

    string out;
    out.len = len;
    out.raw = arena_alloc(&p->string_alloca, len, 1);

    u64 out_i = 0;
    for_urange(i, 0, s.len) {
        char c = s.raw[i];
        if (c == '\\') {
            u64 digit1 = char_to_digit(s.raw[i+1]);
            u64 digit2 = char_to_digit(s.raw[i+2]);
            out.raw[out_i] = digit1 * 16 + digit2;
            i += 2;
        } else {
            out.raw[out_i] = s.raw[i];
        }
        out_i += 1;
    }
    return out;
}

static IrAst* parse_atom(IrParser* p);

static IrAst* parse_list(IrParser* p) {
    
    if (current_token(p).type != TOK_CLOSE_PAREN) {
        IrAst* n = new_ast(p, AST_LIST);
        n->list.this = parse_atom(p);
        if (n->list.this == NULL) return NULL;
        n->list.next = parse_list(p);
        return n;
    }
    return NULL;
}

static IrAst* parse_atom(IrParser* p) {
    IrAst* n = NULL;

    switch (current_token(p).type) {
    case TOK_IDENT:
        n = new_ast(p, AST_IDENT);
        n->token_index = p->index;
        n->ident = token_to_string(p->src, current_token(p));
        advance_token(p);
        break;
    case TOK_NUMERIC:
        n = new_ast(p, AST_NUMERIC);
        n->token_index = p->index;
        n->numeric = parse_integer(token_to_string(p->src, current_token(p)));
        advance_token(p);
        break;
    case TOK_STRING:
        n = new_ast(p, AST_STRING);
        n->token_index = p->index;
        n->string = parse_string(p, token_to_string(p->src, current_token(p)));
        advance_token(p);
        break;
    case TOK_OPEN_PAREN:
        advance_token(p);
        n = parse_list(p);
        advance_token(p);
        break;
    case TOK_CLOSE_PAREN:
        CRASH("unmatched close paren");
        break;
    }

    return n;
}

static void indent(int i) {
    for_range(_, 0, i) printf("  ");
}

static void print_ast(IrAst* n, int i) {
    if (n == NULL) {
        indent(i);
        printf("null\n");
        return;
    }

    switch (n->kind) {
    case AST_IDENT:
        indent(i);
        printf(str_fmt"\n", str_arg(n->ident));
        break;
    case AST_STRING:
        indent(i);
        printf("\""str_fmt"\"\n", str_arg(n->string));
        break;
    case AST_NUMERIC:
        indent(i);
        printf("%llu\n", n->numeric);
        break;
    case AST_LIST:
        indent(i);
        printf("(\n");
        while (n != NULL) {
            print_ast(n->list.this, i + 1);
            n = n->list.next;
        }
        break;
    default:
        break;
    }
}

FeModule* fe_read_ir(string text) {
    IrLexer l = {
        .index = 0,
        .current = text.raw[0],
        .src = text
    };

    da(IrToken) tokens;
    da_init(&tokens, 256);

    IrToken t = next_token(&l);
    while (t.type != TOK_EOF) {
        da_append(&tokens, t);
        t = next_token(&l);
    }
    da_append(&tokens, t);

    IrParser p = {
        .alloca = arena_make(1000*sizeof(IrAst)),
        .string_alloca = arena_make(1000),
        .index = 0,
        .tokens = tokens.at,
        .tokens_len = tokens.len,
        .src = text
    };

    IrAst* ast = parse_atom(&p);

    print_ast(ast, 0);

    return NULL;
}