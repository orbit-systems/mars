#include "../orbit.h"
#include "../error.h"
#include "lexer.h"

dynarr_lib(token)

lexer_state new_lexer(string path, string src) {
    lexer_state lex;
    lex.path = path;
    lex.src = src;
    lex.current_char = src.raw[0];
    dynarr_init(token, &lex.buffer, src.len/3);
    return lex;
}

void construct_token_buffer(lexer_state* restrict lex) {
    if (lex == NULL || is_null_str(lex->src) || is_null_str(lex->path)) {
        CRASH("bad lexer provided to construct_token_buffer");
    }

    while (lex->buffer.base[lex->buffer.len-1].type != tt_EOF) {
        append_next_token(lex);
    }
    dynarr_shrink(token, &lex->buffer);
}

void append_next_token(lexer_state* restrict lex) {

    // if the top token is an EOF, early return
    if (lex->buffer.base[lex->buffer.len-1].type == tt_EOF) {
        return;
    }

    // advance to next significant rune
    loop: while (true) {
        if (lex->cursor >= lex->src.len) {
            dynarr_append(token, 
                &lex->buffer, 
                (token){string_make(&lex->src.raw[lex->src.len-1], 0)}
            );
        }

        switch (current_char(lex)) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\v':
            skip_whitespace(lex);
            break;
        case '/':
            if (peek_char(lex, 1) == '/') {
                skip_until_char(lex, '\n');
            } else if (peek_char(lex, 1) == '*') {
                advance_char_n(lex, 2);
                int final_level = skip_block_comment(lex);
                if (final_level > 0) {
                    error_at_string(lex->path, lex->src, string_make(&lex->src.raw[lex->cursor], 1), 
                        "unclosed block comment"
                    );
                }
            }
        default: break;
        }
    }

}

int skip_block_comment(lexer_state* restrict lex) {
    int level = 1;
    while (level != 0) {
        if (lex->cursor >= lex->src.len) {
            break;
        }
        if (current_char(lex) == '/' && peek_char(lex, 1) == '*') {
            advance_char_n(lex, 2);
            level++;
        }
        else if (current_char(lex) == '*' && peek_char(lex, 1) == '/') {
            advance_char_n(lex, 2);
            level--;
        } else
            advance_char(lex);
    }
    return level;
}

void skip_until_char(lexer_state* restrict lex, char c) {
    while (current_char(lex) != c && lex->cursor < lex->src.len) {
        advance_char(lex);
    }
}

void skip_whitespace(lexer_state* restrict lex) {
    while (true) {
        char r = current_char(lex);
        if (r == ' ' || r == '\t' || r == '\n' || r == '\r' || r == '\v' || lex->cursor >= lex->src.len) {
            return;
        }
        advance_char(lex);
    }
}