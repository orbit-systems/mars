#include "lex.h"
#include "common/util.h"
#include "reporting.h"

Lexer lexer_new(MarsCompiler* marsc, SourceFileId file) {
    Lexer l = {
        .current_file = file,
        .cursor = 0,
        .tokens = vec_new(Token, 256),
        .unclosed_delimiters = vec_new(Token, 64),
        .source = marsc->files[file._].source,
        .marsc = marsc,
    };

    return l;
}

Vec(Token) lexer_destroy_to_tokens(Lexer* l) {
    Vec(Token) ts = l->tokens;
    
    vec_destroy(&l->unclosed_delimiters);

    *l = (Lexer){};
    
    return ts;
}

/// Moves a lexer `l`'s cursor one character forward.
static bool is_eof(Lexer* l) {
    return l->cursor >= l->source.len;
}

/// Move a lexer `l`'s cursor one character forward.
static void advance(Lexer* l) {
    l->cursor += 1;
}

/// Move a lexer `l`'s cursor `n` characters forward.
static void advance_n(Lexer* l, usize n) {
    l->cursor += n;
}

/// Check whether the character `n` bytes past
/// lexer `l`'s cursor matches the character `c`.
static bool peek_match(Lexer* l, usize n, char c) {
    if (l->cursor + n >= l->source.len) {
        return false;
    }
    return l->source.raw[l->cursor + n] == c;
}

/// Peek the character at a lexer's cursor.
/// lexer must not be at the end of the string.
static char current(Lexer* l) {
    assert(!is_eof(l));
    return l->source.raw[l->cursor];
} 

/// Advance a lexer's cursor past whitespace characters, 
/// or until the end of the source text
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

/// Add a token to lexer `l`'s token buffer.
static Token add_token(Lexer* l, TokenKind k) {
    Token t = {
        .kind = k,
        .raw = l->source.raw[l->cursor],
    };

    vec_append(&l->tokens, t);

    return t;
}

/// Add a fake token to lexer `l`'s token buffer.
static Token add_fake_token(Lexer* l, TokenKind k) {
    Token t = {
        .kind = k,
        .fake = true,
        .raw = l->source.raw[l->cursor],
    };

    vec_append(&l->tokens, t);

    return t;
}

/// Add an EOF token to lexer `l`'s token buffer.
static void add_eof_token(Lexer* l) {
    Token t = {
        .kind = TOK_EOF,
        .raw = l->source.raw[l->source.len - 1],
    };

    vec_append(&l->tokens, t);
}

/// Close unmatched parens/braces/brackets and create reports until a delimiter of `open` is found.

static void close_unclosed_delim(Lexer* l, TokenKind open) {

    Report* report = nullptr;

    while (vec_len(l->unclosed_delimiters) != 0) {
        
        Token t = vec_peek(l->unclosed_delimiters);
        vec_len(l->unclosed_delimiters) -= 1;

        if (t.kind == open) {
            break;
        }


        string msg = strlit("expected ???");
        switch (t.kind) {
        case TOK_OPEN_PAREN:
            msg = strlit("expected `)`");
            add_fake_token(l, TOK_CLOSE_PAREN);
            break;
        case TOK_OPEN_BRACKET:
            msg = strlit("expected `]`");
            add_fake_token(l, TOK_CLOSE_BRACKET);
            break;
        case TOK_OPEN_BRACE:
            msg = strlit("expected `}`");
            add_fake_token(l, TOK_CLOSE_BRACE);
            break;
        default:
            UNREACHABLE;
        }

        if (report == nullptr) {
            report = report_new(
                REPORT_ERROR, 
                strlit("unclosed delimiter(s)"), 
                &l->marsc->files
            );
        }
        report_add_label(report, REPORT_LABEL_PRIMARY, l->current_file, l->cursor, l->cursor + 1, msg);
    }
    if (report != nullptr) {
        vec_append(&l->marsc->reports, report);
    }
}

bool lexer_next_token(Lexer* l) {
    skip_whitespace(l);
    if (is_eof(l)) {
        l->cursor -= 1;
        close_unclosed_delim(l, TOK_EOF);
        l->cursor += 1;
        add_eof_token(l);
        return false;
    }

    switch (current(l)) {
    case '\n':
        advance(l);
        break;
    case '(':
        vec_append(&l->unclosed_delimiters, add_token(l, TOK_OPEN_PAREN));
        advance(l);
        break;
    case '[':
        vec_append(&l->unclosed_delimiters, add_token(l, TOK_OPEN_BRACKET));
        advance(l);
        break;
    case '{':
        vec_append(&l->unclosed_delimiters, add_token(l, TOK_OPEN_BRACE));
        advance(l);
        break;
    case ')':
        close_unclosed_delim(l, TOK_OPEN_PAREN);
        add_token(l, TOK_CLOSE_PAREN);
        advance(l);
        break;
    case ']':
        close_unclosed_delim(l, TOK_OPEN_BRACKET);
        add_token(l, TOK_CLOSE_BRACKET);
        advance(l);
        break;
    case '}':
        close_unclosed_delim(l, TOK_OPEN_BRACE);
        add_token(l, TOK_CLOSE_BRACE);
        advance(l);
        break;
    case '#':
        add_token(l, TOK_HASH);
        advance(l);
        break;
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
        TODO("encountered unhandled character '%c'\n", current(l));
    }

    return true;
}
