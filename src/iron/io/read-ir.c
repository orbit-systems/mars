#include "read-ir.h"

// read IR from its textual representation.

static Token next_token(Lexer* l);

static FeModule* read_module(Ast* bruh) {
    TODO("");
    return NULL;
}

FeModule* fe_read_module(string text) {
    Lexer l = {
        .index = 0,
        .current = text.raw[0],
        .src = text
    };

    da(Token) tokens;
    da_init(&tokens, 256);

    Token t = next_token(&l);
    while (t.type != TOK_EOF) {
        da_append(&tokens, t);
        t = next_token(&l);
    }
    da_append(&tokens, t);

    Parser p = {
        .node_alloca = arena_make(1000 * sizeof(Ast)),
        .tokens = tokens.at,
        .tokens_len = tokens.len,
        .index = 0,
        .current = tokens.at[0],
        .text = text,
    };

    return read_module(parse(&p));
}

////////////////////////////////// lexer //////////////////////////////////

static void advance(Lexer* l) {
    if (l->index <= l->src.len) {
        l->index++;
        l->current = l->src.raw[l->index];
    } 
}

static bool is_eof(Lexer* l) {
    return l->index > l->src.len;
}

static Token make_eof(Lexer* l) {
    return (Token){l->index, 0, TOK_EOF};
}

static bool is_whitespace(Lexer* l) {
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

static bool is_numeric(Lexer* l) {
    return '0' <= l->current && l->current <= '9';
}

static bool is_special(Lexer* l) {
    return is_whitespace(l) || l->current == '(' || l->current == ')' || l->current == '"';
}

static Token next_token(Lexer* l) {
    while (is_whitespace(l)) advance(l);

    if (is_eof(l)) return make_eof(l);

    Token t;
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
        if (t.len != l->index - t.index) {
            CRASH("token length exceeds 2^15 characters");
        }
        break;
    default:
        t.index = l->index;
        t.type = TOK_NUMERIC;
        while (!is_eof(l) && !is_special(l)) {
            if (!is_numeric(l)) t.type = TOK_IDENT;
            advance(l);
        }
        t.len = l->index - t.index;
        if (t.len != l->index - t.index) {
            CRASH("token length exceeds 2^15 characters");
        }
        break;
    }

    return t;
}

////////////////////////////////// parser //////////////////////////////////

static void* new_ast(Parser* p, u8 kind) {
    Ast* n;
    switch (kind) {
    case AST_NULL:    n = arena_alloc(&p->node_alloca, sizeof(Ast),        alignof(Ast));        break;
    case AST_NUMERIC: n = arena_alloc(&p->node_alloca, sizeof(AstNumeric), alignof(AstNumeric)); break;
    case AST_STRING:  n = arena_alloc(&p->node_alloca, sizeof(AstString),  alignof(AstString));  break;
    case AST_IDENT:   n = arena_alloc(&p->node_alloca, sizeof(AstIdent),   alignof(AstIdent));   break;
    case AST_LIST:    n = arena_alloc(&p->node_alloca, sizeof(AstList),    alignof(AstList));    break;
    default:
        CRASH("");
        break;
    }
    n->kind = kind;
    n->token_index = p->index;
    return n;
}

static string token_string(Parser* p, Token t) {
    string s = {
        .raw = p->text.raw + t.index,
        .len = t.len,
    };
    return s;
}

static Token get_token(Parser* p, Ast* n) {
    if (p->tokens_len <= n->token_index) return (Token){0};
    return p->tokens[n->token_index];
}

static void advance_token(Parser* p) {
    if (p->index != p->tokens_len - 1) {
        p->index++;
        p->current = p->tokens[p->index];
    }
}

static u64 numeric_value(string s) {
    u64 val = 0;
    for_range(i, 0, s.len) {
        val = val * 10 + (s.raw[i] - '0');
    }
    return val;
}

static u8 hex_value(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a';
    if ('A' <= c && c <= 'F') return c - 'A';
    CRASH("invalid hex digit");
}

static string string_value(string s) {
    s.len -= 2;
    s.raw += 1; // trim ""
    
    string val = {0};
    for_range(i, 0, s.len) {
        char c = s.raw[i];
        if (c == '\\') {
            if (i + 2 >= s.len) CRASH("unfinished escape sequence");
            i += 2;
        }
        val.len += 1;
    }

    val = string_alloc(val.len);

    u64 val_cursor = 0;
    for_range(i, 0, s.len) {
        char c = s.raw[i];
        if (c == '\\') {
            u8 v = hex_value(s.raw[i + 1]);
            v   += hex_value(s.raw[i + 2]);
            val.raw[val_cursor] = v;
            i += 2;
        } else {
            val.raw[val_cursor] = c;
        }
        val_cursor += 1;
    }
    return val;
}

static Ast* parse_list(Parser* p) {
    if (p->current.type == TOK_CLOSE_PAREN) {
        Ast* n = new_ast(p, AST_NULL);
        return n;
    }
    if (p->current.type == TOK_EOF) {
        CRASH("unclosed list");
    }

    AstList* n = new_ast(p, AST_LIST);
    n->this = parse(p);
    n->base.token_index = n->this->token_index;
    n->next = parse_list(p);
    return (Ast*) n;
}

Ast* parse(Parser* p) {
    
    // string tokenstr = token_string(p, p->current);
    // printf("-- "str_fmt"\n", str_arg(tokenstr));

    Ast* n = NULL;
    switch (p->current.type) {
    case TOK_EOF:
        n = new_ast(p, AST_NULL);
        break;
    case TOK_OPEN_PAREN:
        // u64 token_index = p->index;
        advance_token(p);
        n = parse_list(p);
        // n->token_index = token_index;
        advance_token(p);
        break;
    case TOK_IDENT:
        n = new_ast(p, AST_IDENT);
        advance_token(p);
        break;
    case TOK_NUMERIC:
        n = new_ast(p, AST_NUMERIC);
        AstNumeric* numeric = (AstNumeric*) n;
        numeric->value = numeric_value(token_string(p, p->current));
        advance_token(p);
        break;
    case TOK_STRING:
        n = new_ast(p, AST_STRING);
        AstString* str = (AstString*) n;
        str->value = string_value(token_string(p, p->current));
        advance_token(p);
        break;
    case TOK_CLOSE_PAREN:
        CRASH("unmatched )");
        break;
    }

    return n;
}