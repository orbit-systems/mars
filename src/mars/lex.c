#include "lex.h"

Lexer lexer_new(MarsCompiler* compiler, SourceFileId file) {

    Lexer l = {
        .current_file = file,
        .cursor = 0,
        .tokens = vec_new(Token, 256),
        .unclosed_delimiters = vec_new(Token, 64),
        .source = compiler->files[file._].source,
    };

    return l;
}

Vec(Token) lexer_to_tokens(Lexer* l) {
    Vec(Token) ts = l->tokens;
    vec_destroy(&l->unclosed_delimiters);
    *l = (Lexer){};
    return ts;
}

static bool is_eof(Lexer* l) {
    return l->cursor >= l->source.len;
}

static void advance(Lexer* l) {
    l->cursor += 1;
}

static void advance_n(Lexer* l, usize n) {
    l->cursor += n;
}

static bool peek_match(Lexer* l, usize n, char c) {
    if (l->cursor + n >= l->source.len) {
        return false;
    }
    return l->source.raw[l->cursor + n] == c;
    // l->cursor += n;
}

static char current(Lexer* l) {
    assert(!is_eof(l));
    return l->source.raw[l->cursor];
} 

static void skip_whitespace(Lexer* l) {
    while (!is_eof(l)) {
        switch (current(l)) {
        case '\v':
        case '\t':
        case '\r':
        case ' ':
            advance(l);
            continue;
        default:
            return;
        }
    }
}

static Token add_token(Lexer* l, TokenKind k) {
    Token t = {
        .kind = k,
        .raw = l->source.raw[l->cursor],
    };

    vec_append(&l->tokens, t);

    return t;
}

static void add_eof_token(Lexer* l) {
    Token t = {
        .kind = TOK_EOF,
        .raw = l->source.raw[l->source.len - 1],
    };

    vec_append(&l->tokens, t);
}

static bool has_matching_pair(Lexer* l, char c) {
    
}

static bool next_token(Lexer* l) {
    skip_whitespace(l);
    if (is_eof(l)) {
        add_eof_token(l);
        return false;
    }

    switch (current(l)) {
    case '(': vec_append(&l->unclosed_delimiters, add_token(l, TOK_OPEN_PAREN));    advance(l); break;
    case '[': vec_append(&l->unclosed_delimiters, add_token(l, TOK_OPEN_BRACKET));  advance(l); break;
    case '{': vec_append(&l->unclosed_delimiters, add_token(l, TOK_OPEN_BRACE));    advance(l); break;
    case ')': 
        
        add_token(l, TOK_CLOSE_PAREN);
        advance(l);
        break;
    case ']': add_token(l, TOK_CLOSE_BRACKET); advance(l); break;
    case '}': add_token(l, TOK_CLOSE_BRACE);   advance(l); break;
    case '#': add_token(l, TOK_HASH);  advance(l); break;
    case ':':
        if (peek_match(l, 1, ':')) {
            add_token(l, TOK_COLON_COLON); 
            advance_n(l, 2);
            break;
        }
        add_token(l, TOK_COLON); 
        advance(l); 
        break;
    case ',': add_token(l, TOK_COMMA); advance(l); break;
    case '.': add_token(l, TOK_DOT);   advance(l); break;
    case '^': add_token(l, TOK_CARET); advance(l); break;
    case '!': 
        if (peek_match(l, 1, '=')) {
            add_token(l, TOK_NOT_EQ);
            advance_n(l, 2);
            break;
        }
        add_token(l, TOK_EXCLAM); 
        advance(l); 
        break;
    case '?': add_token(l, TOK_QUESTION); advance(l); break;
    case '=': 
        if (peek_match(l, 1, '=')) {
            add_token(l, TOK_EQ_EQ);
            advance_n(l, 2);
            break;
        }
        add_token(l, TOK_EQ); 
        advance(l); 
        break;

    default:
        assert(false);
    }
}
