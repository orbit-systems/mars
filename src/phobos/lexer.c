#include "../orbit.h"
#include "lexer.h"

lexer_state new_lexer(string path, string src) {
    lexer_state lex;
    lex.path = path;
    lex.src = src;
    lex.current_char = src.raw[0];
    dynarr_init(token, &lex.buffer, src.len/3);
    return lex;
}

void construct_token_buffer(lexer_state* lex) {
    if (lex == NULL || is_null_str(lex->src) || is_null_str(lex->path)) {
        CRASH("bad lexer provided to construct_token_buffer");
    }

    while (lex->buffer.base[lex->buffer.len-1].type != tt_EOF) {
        append_next_token(lex);
    }
    dynarr_shrink(token, &lex->buffer);
}

void append_next_token(lexer_state* lex) {
}