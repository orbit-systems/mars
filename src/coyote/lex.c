#include "lex.h"
#include "common/strmap.h"
#include "common/util.h"
#include "common/vec.h"

const char* token_kind[TOK__COUNT] = {

    [TOK__INVALID]  = "[invalid]",
    [TOK_EOF]       = "EOF",

    [TOK_NEWLINE]   = "newline",
    [TOK_HASH]      = "#",

    [TOK_OPEN_PAREN]    = "(",
    [TOK_CLOSE_PAREN]   = ")",
    [TOK_OPEN_BRACE]    = "{",
    [TOK_CLOSE_BRACE]   = "}",
    [TOK_OPEN_BRACKET]  = "[",
    [TOK_CLOSE_BRACKET] = "]",

    [TOK_COLON]     = ":",
    [TOK_COMMA]     = ",",
    [TOK_EQ_EQ]     = "==",
    [TOK_NOT_EQ]    = "!=",
    [TOK_AND]       = "&=",
    [TOK_OR]        = "|",
    [TOK_LESS]      = "<",
    [TOK_GREATER]   = ">",
    [TOK_LESS_EQ]   = "<=",
    [TOK_GREATER_EQ] = ">=",
    [TOK_PLUS]      = "+",
    [TOK_MINUS]     = "-",
    [TOK_MUL]       = "*",
    [TOK_DIV]       = "/",
    [TOK_REM]       = "%",
    [TOK_DOT]       = ".",
    [TOK_AT]        = "@",
    [TOK_XOR]       = "$",
    [TOK_LSHIFT]    = "<<",
    [TOK_RSHIFT]    = ">>",
    [TOK_TILDE]     = "~",
    [TOK_VARARG]    = "...",
    [TOK_CARET]     = "^",
    [TOK_CARET_DOT] = "^.",

    [TOK_EQ]        = "=",
    [TOK_PLUS_EQ]   = "+=",
    [TOK_MINUS_EQ]  = "-=",
    [TOK_MUL_EQ]    = "*=",
    [TOK_DIV_EQ]    = "/=",
    [TOK_REM_EQ]    = "%=",
    [TOK_AND_EQ]    = "&=",
    [TOK_OR_EQ]     = "|=",
    [TOK_XOR_EQ]    = "$=",
    [TOK_LSHIFT_EQ] = "<<=",
    [TOK_RSHIFT_EQ] = ">>=",

    [TOK_IDENTIFIER] = "identifier",

    [TOK_CHAR]      = "char",
    [TOK_STRING]    = "string",
    [TOK_INTEGER]   = "integer",

    #define T(kw) [TOK_KW_##kw] = #kw,
        _LEX_KEYWORDS_
    #undef T

    [TOK_PREPROC_MACRO_PASTE]     = "[[MACRO_PASTE]]",
    // [TOK_PREPROC_MACRO_ARG_PASTE] = "[[MACRO_ARG_PASTE]]",
    [TOK_PREPROC_DEFINE_PASTE]    = "[[DEFINE_PASTE]]",
    [TOK_PREPROC_INCLUDE_PASTE]   = "[[INCLUDE_PASTE]]",
    [TOK_PREPROC_PASTE_END]       = "[[PASTE_END]]",

    [TOK_PREPROC_SECTION]      = "#SECTION",
    [TOK_PREPROC_ENTERSECTION] = "#ENTERSECTION",
    [TOK_PREPROC_LEAVESECTION] = "#LEAVESECTION",
    [TOK_PREPROC_ASM]          = "#ASM",
};

static void advance(Lexer* l) {
    l->cursor++;
    if (l->cursor >= l->src.len) {
        l->eof = true;
        l->current = '\0';
    } else {
        l->current = l->src.raw[l->cursor];
    }
}

static char peek(Lexer* l, usize n) {
    usize cursor = l->cursor + n;
    if (cursor >= l->src.len) {
        return '\0';
    } else {
        return l->src.raw[cursor];
    }
}

static void advance_n(Lexer* l, usize n) {
    l->cursor += n;
    if (l->cursor >= l->src.len) {
        l->eof = true;
        l->current = '\0';
    } else {
        l->current = l->src.raw[l->cursor];
    }
}

static Token eof_token(Lexer* l) {
    Token t;
    t.kind = TOK_EOF;
    t.len = 0;
    t.raw = (i64)&l->src.raw[l->src.len];
    return t;
}

static Token construct_and_advance(Lexer* l, u8 kind, u8 len) {
    Token t;
    t.kind = kind;
    t.len = len;
    t.raw = (i64)&l->src.raw[l->cursor];
    assert((char*)(i64)t.raw == &l->src.raw[l->cursor]);
    advance_n(l, len);
    return t;
}

string tok_span(Token t) {
    return (string){
        .raw = tok_raw(t),
        .len = t.len
    };
}

// perfect hash function
// WARNING: THIS HAS TO BE REMADE EVERY TIME A KEYWORD IS ADDED
// USE SCRIPTS/HASHGEN FOR THIS
usize hashfunc(const char* key, usize key_len) {
    usize hash = 4311;

    for (usize i = 0; i < key_len; ++i) {
        hash ^= key[i];
        hash *= 975;
    }
    return hash % 121;
}


static const char* keyword_table[121];
static u8 keyword_len_table[121];
static u8 keyword_code_table[121];

static u8 lex_categorize_keyword(char* s, size_t len) {
    u8 index = hashfunc(s, len);

    if (keyword_len_table[index] != len) {
        return TOK_IDENTIFIER;
    }
    if (strncmp(keyword_table[index], s, len) != 0) {
        return TOK_IDENTIFIER;
    }

    return keyword_code_table[index];
}

static bool is_alphabetic(char c) {
    return 
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        c == '_';
}
static bool is_alphanumeric(char c) {
    return 
        ('a' <= c && c <= 'z') ||
        ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9') ||
        c == '_';
}
static bool is_numeric(char c) {
    return ('0' <= c && c <= '9');
}
static bool is_whitespace(char c) {
    switch (c) {
    // case '\n':
    case '\0': case ' ': case '\t': case '\r': case '\v':
        return true;
    default:
        return false;
    }
}

static Lexer lexer_from_string(string src) {
    Lexer l;
    l.cursor = 0;
    l.src = src;
    if (src.len == 0) {
        l.eof = true;
        l.current = '\0';
    } else {
        l.eof = false;
        l.current = src.raw[0];
    }
    return l;
}


static void skip_whitespace(Lexer* l) {
    while (!l->eof && is_whitespace(l->current)) {
        advance(l);
    }
}

static char* span_begin(Lexer* l) {
    char* c = &l->src.raw[l->cursor];
    while (is_whitespace(*c)) {
        c++;
    }
    return c;
}

static string span_end(Lexer* l, char* begin) {
    char* c = &l->src.raw[l->cursor];
    while (is_whitespace(*c)) {
        c--;
    }
    return (string){.raw = begin, .len = (usize)c - (usize)begin};
}

// lex next token without preprocessor modification.
static Token lex_next_raw(Lexer* l) {
    skip_whitespace(l);

    if (l->eof) return eof_token(l);
    switch (l->current) {
        case '#':  return construct_and_advance(l, TOK_HASH, 1);
        case '\n': return construct_and_advance(l, TOK_NEWLINE, 1);

        case '(': return construct_and_advance(l, TOK_OPEN_PAREN, 1);
        case ')': return construct_and_advance(l, TOK_CLOSE_PAREN, 1);
        case '{': return construct_and_advance(l, TOK_OPEN_BRACE, 1);
        case '}': return construct_and_advance(l, TOK_CLOSE_BRACE, 1);
        case '[': return construct_and_advance(l, TOK_OPEN_BRACKET, 1);
        case ']': return construct_and_advance(l, TOK_CLOSE_BRACKET, 1);

        case ':': return construct_and_advance(l, TOK_COLON, 1);
        case ',': return construct_and_advance(l, TOK_COMMA, 1);
        case '@': return construct_and_advance(l, TOK_AT, 1);
        case '~': return construct_and_advance(l, TOK_TILDE, 1);
        case '^':
            if (peek(l, 1) == '.')
                return construct_and_advance(l, TOK_CARET_DOT, 2);
            else
                return construct_and_advance(l, TOK_CARET, 1);

        case '.':
            if (peek(l, 1) == '.' && peek(l, 2) == '.')
                return construct_and_advance(l, TOK_VARARG, 3);
            else
                return construct_and_advance(l, TOK_DOT, 1);

        case '+':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_PLUS_EQ, 2);
            else
                return construct_and_advance(l, TOK_PLUS, 1);
        case '-':
            if (is_numeric(peek(l, 1))) {
                usize length = 2;
                while (is_alphanumeric(peek(l, length))) {
                    ++length;
                }
        
                if (length > LEX_MAX_TOKEN_LEN) {
                    TODO("token is longer than max token len");
                }
                return construct_and_advance(l, TOK_INTEGER, length);
            } else if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_MINUS_EQ, 2);
            else
                return construct_and_advance(l, TOK_MINUS, 1);
        case '*':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_MUL_EQ, 2);
            else
                return construct_and_advance(l, TOK_MUL, 1);
        case '/':
            if (peek(l, 1) == '/') {
                while (!l->eof && l->current != '\n') {
                    advance(l);
                }
                return lex_next_raw(l); // tail-call hopefully
            } else if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_DIV_EQ, 2);
            else
                return construct_and_advance(l, TOK_DIV, 1);
        case '%':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_REM_EQ, 2);
            else
                return construct_and_advance(l, TOK_REM, 1);
        case '$':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_XOR_EQ, 2);
            else
                return construct_and_advance(l, TOK_XOR, 1);
        case '&':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_AND_EQ, 2);
            else
                return construct_and_advance(l, TOK_AND, 1);
        case '|':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_OR_EQ, 2);
            else
                return construct_and_advance(l, TOK_OR, 1);

        case '=':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_EQ_EQ, 2);
            else
                return construct_and_advance(l, TOK_EQ, 1);
        case '!':
            if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_NOT_EQ, 2);
            else
                break;

        case '<':
            if (peek(l, 1) == '<') {
                if (peek(l, 2) == '=')
                    return construct_and_advance(l, TOK_LSHIFT_EQ, 3);
                else
                    return construct_and_advance(l, TOK_LSHIFT, 2);
            } 
            else if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_LESS_EQ, 2);
            else
                return construct_and_advance(l, TOK_LESS, 1);
        
        case '>':
            if (peek(l, 1) == '>') {
                if (peek(l, 2) == '=')
                    return construct_and_advance(l, TOK_RSHIFT_EQ, 2);
                else
                    return construct_and_advance(l, TOK_RSHIFT, 2);
            } 
            else if (peek(l, 1) == '=')
                return construct_and_advance(l, TOK_GREATER_EQ, 2);
            else
                return construct_and_advance(l, TOK_GREATER, 1);
        case '\"': {
            usize length = 1;
            bool seen_slash = false;
            char c = peek(l, length);
            while (c != '\"' || seen_slash) {
                ++length;
                seen_slash = c == '\\';
                c = peek(l, length);
            }
            // length--;
            l->cursor++;
            if (length > LEX_MAX_TOKEN_LEN) {
                TODO("token is longer than max token len");
            }
            Token string_token = construct_and_advance(l, TOK_STRING, length-1);
            advance(l);
            return string_token;
        }
        case '\'': {
            usize length = 1;
            bool seen_slash = false;
            char c = peek(l, length);
            while (c != '\'' || seen_slash) {
                if (seen_slash) {
                    seen_slash = false;
                } else {
                    seen_slash = c == '\\';
                }
                
                ++length;
                c = peek(l, length);
            }
            length++;
            if (length > LEX_MAX_TOKEN_LEN) {
                TODO("token is longer than max token len");
            }
            return construct_and_advance(l, TOK_CHAR, length);
        }
    }

    if (is_alphabetic(l->current)) {
        usize length = 1;
        while (is_alphanumeric(peek(l, length))) {
            ++length;
        }

        if (length > LEX_MAX_TOKEN_LEN) {
            TODO("token is longer than max token len");
        }

        u8 kind = lex_categorize_keyword(&l->src.raw[l->cursor], length);
        return construct_and_advance(l, kind, length);
    }

    if (is_numeric(l->current)) {
        usize length = 1;
        while (is_alphanumeric(peek(l, length))) {
            ++length;
        }

        if (length > LEX_MAX_TOKEN_LEN) {
            TODO("token is longer than max token len");
        }
        return construct_and_advance(l, TOK_INTEGER, length);
    }

    UNREACHABLE;
}

// ------------------------- PREPROCESSOR ------------------------- 

static Token lex_with_preproc(Lexer* l, Vec(Token)* tokens, PreprocScope* scope);

Vec(Token) macro_arg_pool;
Vec(PreprocVal) preproc_val_pool;

static bool replacement_exists_immediate(string key, PreprocScope* scope) {
    if (scope == nullptr) return false;
    return STRMAP_NOT_FOUND != strmap_get(&scope->map, key);
}

static bool replacement_exists(string key, PreprocScope* scope) {
    if (scope == nullptr) return false;
    return replacement_exists_immediate(key, scope) 
        || replacement_exists(key, scope->parent);
}

static void remove_replacement(string key, PreprocScope* scope) {
    strmap_remove(&scope->map, key);
    if (scope->parent != nullptr) {
        remove_replacement(key, scope->parent);
    }
}

static PreprocVal get_replacement_value(string key, PreprocScope* scope) {
    usize val = (usize)strmap_get(&scope->map, key);
    if (val == (usize)STRMAP_NOT_FOUND) {
        if (scope->parent != nullptr) {
            return get_replacement_value(key, scope->parent);
        }
        return (PreprocVal){.kind = PPVAL_NONE};
    }
    return preproc_val_pool.at[val];
}

static void put_replacement_value(string key, PreprocScope* scope, PreprocVal val) {
    vec_append(&preproc_val_pool, val);
    // place at innermost scope
    strmap_put(&scope->map, key, (void*)(preproc_val_pool.len - 1));
}

static CompactString preproc_collect_complex_string(Lexer* l) {
    string span;
    span.raw = &l->src.raw[l->cursor];
    span.len = l->cursor;

    usize bracket_depth = 1;
    while (bracket_depth != 0) {    
        Token t = lex_next_raw(l);
        switch (t.kind) {
        case TOK_OPEN_BRACKET:  ++bracket_depth; break;
        case TOK_CLOSE_BRACKET: --bracket_depth; break;
        default:
            break;
        }
    }

    span.len = l->cursor - span.len - 1;
    if (span.len > COMPACT_STR_MAX_LEN) {
        TODO("error: string too long");
    }
    return to_compact(span);
}

static i64 eval_integer(Token t) {
    i64 val = 0;
    char* raw = tok_raw(t);
    bool is_negative = raw[0] == '-';

    for (usize i = is_negative; i < t.len; ++i) {
        val *= 10;
        isize char_val = raw[i] - '0';
        // if (char_val < 0 || char_val > 9) {
        //     TODO("invalid digit");
        // }
        val += raw[i] - '0';
    }

    return is_negative ? -val : val;
}

static PreprocVal preproc_collect_value(Lexer* l, PreprocScope* scope) {
    PreprocVal v = {0};
    Token t = lex_next_raw(l);
    v.raw = t.raw;
    v.len = t.len;

    switch (t.kind) {
    case TOK_OPEN_BRACKET:
        v.kind = PPVAL_COMPLEX_STRING;
        v.string = preproc_collect_complex_string(l);
        break;
    case TOK_IDENTIFIER:
        ;
        string span = tok_span(t);
        if (replacement_exists(span, scope)) {
            v = get_replacement_value(span, scope);
        } else {
            TODO("error: preprocessor symbol undefined");
        }
        break;
    case TOK_INTEGER:
        v.kind = PPVAL_INTEGER;
        v.integer = eval_integer(t);
        break;
    case TOK_OPEN_PAREN:
        ;
        Token op = lex_next_raw(l);

        if (op.kind == TOK_IDENTIFIER) {
            if (string_eq(tok_span(op), strlit("DEFINED"))) {
                Token ident = lex_next_raw(l);
                if (ident.kind != TOK_IDENTIFIER) {
                    TODO("error: expected identifier");
                }
                v.kind = PPVAL_INTEGER;
                v.integer = replacement_exists(tok_span(ident), scope);
            } else if (string_eq(tok_span(op), strlit("STRCAT"))) {
                PreprocVal lhs = preproc_collect_value(l, scope);
                PreprocVal rhs = preproc_collect_value(l, scope);
                if (lhs.kind != PPVAL_STRING && rhs.kind != PPVAL_STRING) {
                    TODO("error: expected string");
                }

                string newstr;
                newstr.len = lhs.string.len + rhs.string.len;
                newstr.raw = arena_alloc(l->arena, newstr.len, 1);
                if (newstr.len > COMPACT_STR_MAX_LEN) {
                    TODO("error: string too long");
                }

                memcpy(newstr.raw, (void*)(i64)lhs.string.raw, lhs.string.len);
                memcpy(newstr.raw + lhs.string.len, (void*)(i64)rhs.string.raw, rhs.string.len);

                v.kind = PPVAL_STRING;
                v.string = to_compact(newstr);

            } else if (string_eq(tok_span(op), strlit("STRCMP"))) {
                PreprocVal lhs = preproc_collect_value(l, scope);
                PreprocVal rhs = preproc_collect_value(l, scope);
                if (lhs.kind != PPVAL_STRING && rhs.kind != PPVAL_STRING) {
                    TODO("error: expected string");
                }
                v.kind = PPVAL_INTEGER;
                v.integer = string_cmp(from_compact(lhs.string), from_compact(rhs.string));
            } else {
                TODO("error: invalid operator");
            }
        } else if ((TOK_PLUS <= op.kind && op.kind <= TOK_GREATER) 
            || op.kind == TOK_KW_AND || op.kind == TOK_KW_OR) 
        {
            PreprocVal lhs = preproc_collect_value(l, scope);
            PreprocVal rhs = preproc_collect_value(l, scope);
            if (lhs.kind != PPVAL_INTEGER || rhs.kind != PPVAL_INTEGER) {
                TODO("error: expected integer");
            }
            v.kind = PPVAL_INTEGER;
            switch (op.kind) {
            case TOK_PLUS:       v.integer = lhs.integer + rhs.integer; break;
            case TOK_MINUS:      v.integer = lhs.integer - rhs.integer; break;
            case TOK_MUL:        v.integer = lhs.integer * rhs.integer; break;
            case TOK_DIV:        v.integer = lhs.integer / rhs.integer; break;
            case TOK_REM:        v.integer = lhs.integer % rhs.integer; break;
            case TOK_AND:        v.integer = lhs.integer & rhs.integer; break;
            case TOK_OR:         v.integer = lhs.integer | rhs.integer; break;
            case TOK_XOR:        v.integer = lhs.integer ^ rhs.integer; break;
            case TOK_LSHIFT:     v.integer = lhs.integer << rhs.integer; break;
            case TOK_RSHIFT:     v.integer = lhs.integer >> rhs.integer; break;
            case TOK_EQ_EQ:      v.integer = lhs.integer == rhs.integer; break;
            case TOK_NOT_EQ:     v.integer = lhs.integer != rhs.integer; break;
            case TOK_LESS_EQ:    v.integer = lhs.integer <= rhs.integer; break;
            case TOK_GREATER_EQ: v.integer = lhs.integer >= rhs.integer; break;
            case TOK_LESS:       v.integer = lhs.integer < rhs.integer; break;
            case TOK_GREATER:    v.integer = lhs.integer > rhs.integer; break;
            case TOK_KW_AND:     v.integer = lhs.integer && rhs.integer; break;
            case TOK_KW_OR:      v.integer = lhs.integer || rhs.integer; break;
            default:
                UNREACHABLE;
            }
        } else if (op.kind == TOK_TILDE) {
            PreprocVal inner = preproc_collect_value(l, scope);
            if (inner.kind != PPVAL_INTEGER) {
                TODO("error: expected integer");
            }
            v.kind = PPVAL_INTEGER;
            v.integer = ~inner.integer;
        } else if (op.kind == TOK_KW_NOT) {
            PreprocVal inner = preproc_collect_value(l, scope);
            if (inner.kind != PPVAL_INTEGER) {
                TODO("error: expected integer");
            }
            v.kind = PPVAL_INTEGER;
            v.integer = !inner.integer;
        } else {
            TODO("error: invalid operator");
        }

        if (lex_next_raw(l).kind != TOK_CLOSE_PAREN) {
            TODO("error: expected )");
        }
        break;
    case TOK_STRING:
        v.kind = PPVAL_STRING;
        v.len = v.string.len = t.len;
        v.raw = v.string.raw = t.raw;
        break;
    default:
        TODO("error: expected value, got token '%s'", token_kind[t.kind]);
    }

    return v;
}

static void preproc_define(Lexer* l, PreprocScope* scope) {
    // consume name
    Token name = lex_next_raw(l);
    if (name.kind != TOK_IDENTIFIER) {
        TODO("error: expected identifier");
    }

    // consume value
    PreprocVal v = preproc_collect_value(l, scope);
    v.source = (CompactString){   
        .len = name.len,
        .raw = name.raw,
    };

    if (replacement_exists_immediate(tok_span(name), scope)) {
        TODO("error: redefinition in current scope");
    }

    put_replacement_value(tok_span(name), scope, v);
}

static void preproc_undefine(Lexer* l, PreprocScope* scope) {
    // consume name
    Token name = lex_next_raw(l);
    if (name.kind != TOK_IDENTIFIER) {
        TODO("error: expected identifier");
    }

    remove_replacement(tok_span(name), scope);
}

static void preproc_macro(Lexer* l, PreprocScope* scope) {
    // consume name
    Token name = lex_next_raw(l);
    if (name.kind != TOK_IDENTIFIER) {
        TODO("error: expected identifier");
    }

    if (replacement_exists_immediate(tok_span(name), scope)) {
        TODO("error: redefinition in current scope");
    }

    if (lex_next_raw(l).kind != TOK_OPEN_PAREN) {
        TODO("error: expected (");
    }

    // consume param list
    usize params_len = 0;
    usize macro_params_index = macro_arg_pool.len;
    for (Token t = lex_next_raw(l); t.kind != TOK_CLOSE_PAREN; t = lex_next_raw(l)) {
        if (t.kind != TOK_IDENTIFIER) {
            TODO("error: expected identifier");
        }

        ++params_len;
        vec_append(&macro_arg_pool, t);

        t = lex_next_raw(l);
        if (t.kind != TOK_COMMA) {
            if (t.kind == TOK_CLOSE_PAREN) {
                break;
            }
            TODO("error: expected , or )");
        }
    }

    // consume body
    PreprocVal body = preproc_collect_value(l, scope);
    if (body.kind != PPVAL_COMPLEX_STRING) {
        TODO("error: expected complex string (using [])");
    }
    vec_append(&preproc_val_pool, body);
    usize body_index = preproc_val_pool.len - 1;

    PreprocVal macro = {
        .kind = PPVAL_MACRO,
        .source = (CompactString){   
            .len = name.len,
            .raw = name.raw,
        },
        .macro = {
            // .scope = scope,
            .body_index = body_index,
            .params_index = macro_params_index,
            .params_len = params_len,
        },
    };
    put_replacement_value(tok_span(name), scope, macro);
}

static void preproc_if(Lexer* l, Vec(Token)* tokens, PreprocScope* scope) {
    
    PreprocVal cond_val = preproc_collect_value(l, scope);
    if (cond_val.kind != PPVAL_INTEGER) {
        TODO("#IF condition is not an integer");
    }
    bool case_succeeds = cond_val.integer != 0;
    // if this case suceeds, we include everything inside and then discard everything else
    if (case_succeeds) {
        // get the rest of this case
        Token last = lex_with_preproc(l, tokens, scope);

        // discard everything else until the last #END
        if (last.kind == TOK_KW_END) {
            return;
        }
        u32 depth = 1;
        while (last.kind != TOK_KW_END || depth != 0) {
            last = lex_next_raw(l);
            if (last.kind == TOK_EOF) {
                TODO("unexpected EOF");
                return;
            }
            if (last.kind != TOK_HASH) {
                continue;
            }
            last = lex_next_raw(l);
            switch (last.kind) {
            case TOK_KW_IF:
                depth++;
                break;
            case TOK_KW_END:
                depth--;
                break;
            case TOK_EOF:
                TODO("unexpected EOF");
                return;
            }
        }
    } else {
        // skip this case.
        // if there's an other case, evaluate it
        u32 depth = 1;
        Token t;
        while (depth != 0) {
            t = lex_next_raw(l);
            if (t.kind == TOK_EOF) {
                break;
            } else if (t.kind != TOK_HASH) {
                continue;
            }

            t = lex_next_raw(l);
            switch (t.kind) {
            case TOK_KW_IF:
                depth++;
                break;
            case TOK_KW_ELSE:
            case TOK_KW_ELSEIF:
                if (depth == 1) {
                    depth = 0;
                }
                break;
            case TOK_KW_END:
                depth--;
                break;
            }
        }
        // printf("%s\n", token_kind[t.kind]);
        switch (t.kind) {
        case TOK_KW_END:
            return;
        case TOK_KW_ELSE:
            ;
            // get the rest
            Token last = lex_with_preproc(l, tokens, scope);

            // expect END
            if (last.kind != TOK_KW_END) {
                TODO("expected #END");
            }
            break;
        case TOK_KW_ELSEIF:
            preproc_if(l, tokens, scope);
            break;
        default:
            TODO("expected #ELSEIF, #ELSE, or #END");
        }
    }
}

static void preproc_include(Lexer* l, Vec(Token)* tokens, PreprocScope* scope) {
    // next thing should be a string literal.

    Token path = lex_next_raw(l);
    
}

static Token preproc_dispatch(Lexer* l, Vec(Token)* tokens, PreprocScope* scope) {

    // todo: add preproc keywords to the hash table

    Token t = lex_next_raw(l);
    switch (t.kind) {
    case TOK_KW_IF:
        preproc_if(l, tokens, scope);
        break;      
    case TOK_KW_ELSE:
    case TOK_KW_ELSEIF:
    case TOK_KW_END:
        // the host lexer needs to take care of this,
        // pass it back.
        return t;
    }
    
    string span = tok_span(t);
    if (string_eq(span, strlit("INCLUDE"))) {
        preproc_include(l, tokens, scope);
    } else if (string_eq(span, strlit("DEFINE"))) {
        preproc_define(l, scope);
    } else if (string_eq(span, strlit("UNDEFINE"))) {
        preproc_undefine(l, scope);
    } else if (string_eq(span, strlit("MACRO"))) {
        preproc_macro(l, scope);
    } else {
        // TODO("error: unrecognized directive");
    }
    return (Token){}; // its fine lol
}

usize emit_depth = 0;
#define MAX_EMIT_DEPTH 64

static PreprocScope global_scope;
static PreprocScope local_scopes[MAX_EMIT_DEPTH];

static void emit_preproc_val(PreprocVal val, Vec(Token)* tokens, PreprocScope* scope) {
    ++emit_depth;
    if (emit_depth > MAX_EMIT_DEPTH) {
        TODO("error: max define/macro depth reached");
    }

    switch (val.kind) {
    case PPVAL_INTEGER:
        if (val.raw != 0) {
            Token t;
            t.kind = TOK_INTEGER;
            t.len = val.len;
            t.raw = val.raw;
            vec_append(tokens, t);
        } else {
            UNREACHABLE;
        }
        break;
    case PPVAL_COMPLEX_STRING:
        ;
        Lexer local_lexer = lexer_from_string(from_compact(val.string));
        // PreprocScope _local_scope_ = {};
        // PreprocScope* local_scope = &_local_scope_;
        PreprocScope* local_scope = &local_scopes[emit_depth - 1];
        local_scope->parent = scope;
        if (val.is_macro_arg) { // prevent name conflicts/infinite recursion bullshit
            local_scope->parent = scope->parent;
        }
        if (local_scope->map.vals) {
            strmap_reset(&local_scope->map);
        } else {
            strmap_init(&local_scope->map, 4);
        }

        lex_with_preproc(&local_lexer, tokens, local_scope);
        // strmap_destroy(&local_scope->map);
        break;
    case PPVAL_STRING:
        ;
        Token t;
        t.kind = TOK_STRING;
        t.len = val.string.len;
        t.raw = val.string.raw;
        vec_append(tokens, t);
        break;
    default:
        UNREACHABLE;
    }
    --emit_depth;
}

static CompactString collect_macro_arg(Lexer* l, PreprocScope* scope) {
    string span;
    span.raw = &l->src.raw[l->cursor];
    span.len = l->cursor;
    usize cap = 0;

    usize bracket_depth = 0;
    for (Token t = lex_next_raw(l);; t = lex_next_raw(l)) {
        
        if (bracket_depth == 0 && (t.kind == TOK_COMMA || t.kind == TOK_CLOSE_PAREN)) {
            break;
        }


        switch (t.kind) {
        // case TOK_IDENTIFIER:
            // if (!replacement_exists(tok_span(t), scope)) {
            //     break;
            // }
            // // we have to expand this. break out the backing buffer if necessary
            // if (cap == 0) {

            // }
        case TOK_OPEN_PAREN: ++bracket_depth; break;
        case TOK_CLOSE_PAREN: --bracket_depth; break;
        }
    }

    span.len = l->cursor - span.len - 1;
    if (span.len > COMPACT_STR_MAX_LEN) {
        TODO("error: argument too long");
    }
    return to_compact(span);
}

static void collect_macro_args_and_emit(Lexer* l, PreprocVal macro, Vec(Token)* tokens, PreprocScope* scope) {
    ++emit_depth;
    if (emit_depth > MAX_EMIT_DEPTH) {
        TODO("error: max define/macro depth reached");
    }
    assert(macro.kind == PPVAL_MACRO);

    // PreprocScope _local_scope_ = {};
    // PreprocScope* local_scope = &_local_scope_;
    PreprocScope* local_scope = &local_scopes[emit_depth - 1];
    local_scope->parent = scope;
    if (local_scope->map.vals) {
        strmap_reset(&local_scope->map);
    } else {
        strmap_init(&local_scope->map, 4);
    }

    // collect args as complex strings, define them in the new scope

    Token t = lex_next_raw(l);
    if (t.kind != TOK_OPEN_PAREN) {
        TODO("error: expected (");
    }

    usize saved_ppv_len = preproc_val_pool.len;

    if (macro.macro.params_len == 0) {
        Token t = lex_next_raw(l);
        if (t.kind != TOK_CLOSE_PAREN) {
            TODO("error: too many parameters");
        }
    } else {
        // consume arg list
        usize arg_len = 0;
        while (l->src.raw[l->cursor - 1] != ')') {
            CompactString arg_span = collect_macro_arg(l, scope);
            PreprocVal arg = {
                .kind = PPVAL_COMPLEX_STRING,
                .is_macro_arg = true,
                .string = arg_span,
            };

            if (arg_len >= macro.macro.params_len) {
                TODO("error: too many parameters, expected %u", macro.macro.params_len);
            }
            string param_name = tok_span(macro_arg_pool.at[macro.macro.params_index + arg_len]);
            put_replacement_value(param_name, local_scope, arg);
            ++arg_len;
        }
    }

    PreprocVal body = preproc_val_pool.at[macro.macro.body_index];
    Lexer local_lexer = lexer_from_string(from_compact(body.string));
    lex_with_preproc(&local_lexer, tokens, local_scope);

    strmap_destroy(&local_scope->map);
    preproc_val_pool.len = saved_ppv_len; // allow reuse of pool space
    --emit_depth;
}

// ------------------------- LEX THAT FILE DAMMIT ------------------------- 

static Token preproc_token(u8 kind, string span) {
    Token t;
    t.generated = true;
    t.kind = kind;
    t.raw = (i64)span.raw;
    t.len = span.len;
    return t;
}

static Token preproc_token_no_span(u8 kind) {
    Token t;
    t.generated = true;
    t.kind = kind;
    t.len = 0;
    return t;
}

// returns the last token it sees.
static Token lex_with_preproc(Lexer* l, Vec(Token)* tokens, PreprocScope* scope) {
    Token t = lex_next_raw(l);
    for (; t.kind != TOK_EOF; t = lex_next_raw(l)) {
        switch (t.kind) {
        case TOK_IDENTIFIER:
            ;
            string span = tok_span(t);
            if (replacement_exists(span, scope)) {
                PreprocVal val = get_replacement_value(span, scope);
                if (val.kind == PPVAL_MACRO) {
                    // string from_span;
                    // from_span.len = val.len;
                    // from_span.raw = (char*)(i64)val.raw;
                    vec_append(tokens, preproc_token(TOK_PREPROC_MACRO_PASTE, from_compact(val.source)));
                    collect_macro_args_and_emit(l, val, tokens, scope);
                    span.len = (usize)l->cursor - (usize)span.raw + (usize)l->src.raw;
                    vec_append(tokens, preproc_token(TOK_PREPROC_PASTE_END, span));
                } else {
                    if (!val.is_macro_arg) {
                        vec_append(tokens, preproc_token(TOK_PREPROC_DEFINE_PASTE, from_compact(val.source)));
                    }
                    emit_preproc_val(val, tokens, scope);
                    if (!val.is_macro_arg) {
                        vec_append(tokens, preproc_token(TOK_PREPROC_PASTE_END, span));
                    }
                }
                continue;
            }
            break;
        case TOK_HASH:
            ;
            Token t = preproc_dispatch(l, tokens, scope);
            switch (t.kind) {
            case 0:
                break;
            case TOK_KW_ELSE:
            case TOK_KW_ELSEIF:
            case TOK_KW_END:
                return t; // pass it up the stack
            default:
                UNREACHABLE;
            }
            continue;
        }
        vec_append(tokens, t);
    }
    return t;
}

Parser lex_entrypoint(SrcFile* f) {
    // init keyword perfect hash table
    for (usize i = TOK__KEYWORDS_BEGIN + 1; i < TOK__KEYWORDS_END; ++i) {
        const char* keyword = token_kind[i];
        u8 hash = hashfunc(keyword, strlen(keyword));
        if (keyword_table[hash]) {
            TODO("keyword table hash collision");
        }
        keyword_table[hash] = keyword;
        keyword_len_table[hash] = strlen(keyword);
        keyword_code_table[hash] = i;
    }

    Arena arena;
    arena_init(&arena);

    // init macro info arena
    macro_arg_pool = vec_new(Token, 128);
    preproc_val_pool = vec_new(PreprocVal, 128);
    
    Vec(Token) tokens = vec_new(Token, 512);
    Lexer l = lexer_from_string(f->src);
    l.arena = &arena;
    strmap_init(&global_scope.map, 64);

    lex_with_preproc(&l, &tokens, &global_scope);

    vec_append(&tokens, eof_token(&l));

    strmap_destroy(&global_scope.map);
    vec_destroy(&preproc_val_pool);
    vec_destroy(&macro_arg_pool);

    vec_shrink(&tokens);

    Parser ctx = {
        .tokens = tokens.at,
        .tokens_len = tokens.len,
        .sources = vecptr_new(SrcFile, 16),
        .cursor = 0,
        .arena = arena,
    };
    ctx.global_scope = arena_alloc(&ctx.arena, sizeof(ParseScope), alignof(ParseScope));
    ctx.global_scope->sub = nullptr;
    ctx.global_scope->super = nullptr;
    strmap_init(&ctx.global_scope->map, 128);
    ctx.current_scope = ctx.global_scope;
    
    arena_init(&ctx.entities);

    for_n(i, 0, tokens.len) {
        if (tokens.at[i].kind < TOK__PARSE_IGNORE) {
            ctx.current = tokens.at[i];
            ctx.cursor = i;
            break;
        }
    }

    vec_append(&ctx.sources, f);

    return ctx;
}
