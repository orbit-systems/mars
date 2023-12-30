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
        // printf("(%3d %s)\n",lex->buffer.base[lex->buffer.len-1].type, to_cstring(lex->buffer.base[lex->buffer.len-1].text));
    }
    dynarr_shrink(token, &lex->buffer);
}

void append_next_token(lexer_state* restrict lex) {

    // if the top token is an EOF, early return
    if (lex->buffer.base[lex->buffer.len-1].type == tt_EOF) {
        return;
    }
    
    // advance to next significant char
    while (true) {
        switch (current_char(lex)) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\v':
            advance_char(lex);
            break;
        case '/':
            if (peek_char(lex, 1) == '/') {
                skip_until_char(lex, '\n');
            } else if (peek_char(lex, 1) == '*') {
                advance_char_n(lex, 2);
                int final_level = skip_block_comment(lex);
                if (final_level != 0) {
                    error_at_string(lex->path, lex->src, string_make(&lex->src.raw[lex->cursor], 1), 
                        "unclosed block comment"
                    );
                }
            } else goto skip_insignificant_end;
            break;
        default:
            goto skip_insignificant_end;
        }
    }
    skip_insignificant_end:

    if (lex->cursor >= lex->src.len) {
        dynarr_append(token, 
            &lex->buffer, 
            ((token){substring_len(lex->src, lex->cursor, 1), tt_EOF})
        );
        return;
    }

    u64 beginning_cursor = lex->cursor;
    token_type this_type = 0;
    if (can_start_identifier(current_char(lex))) {
        this_type = scan_ident_or_keyword(lex);
    } else if (can_start_number(current_char(lex))) {
        this_type = scan_number(lex);
    } else if (current_char(lex) == '\"' || current_char(lex) == '\'') {
        this_type = scan_string_or_char(lex);
    } else {
        this_type = scan_operator(lex);
    }

    dynarr_append_token(&lex->buffer, (token){
        .text = substring(lex->src, beginning_cursor, lex->cursor), 
        .type = this_type,
    });
}

token_type scan_ident_or_keyword(lexer_state* restrict lex) {
    u64 beginning = lex->cursor;
    
    advance_char(lex);
    while (can_be_in_identifier(current_char(lex))) advance_char(lex);

    string word = substring(lex->src, beginning, lex->cursor);

    if (string_eq(word, to_string("int")))          return tt_type_keyword_int;
    if (string_eq(word, to_string("i8")))           return tt_type_keyword_i8;
    if (string_eq(word, to_string("i16")))          return tt_type_keyword_i16;
    if (string_eq(word, to_string("i32")))          return tt_type_keyword_i32;
    if (string_eq(word, to_string("i64")))          return tt_type_keyword_i64;
    if (string_eq(word, to_string("uint")))         return tt_type_keyword_uint;
    if (string_eq(word, to_string("u8")))           return tt_type_keyword_u8;
    if (string_eq(word, to_string("u16")))          return tt_type_keyword_u16;
    if (string_eq(word, to_string("u32")))          return tt_type_keyword_u32;
    if (string_eq(word, to_string("u64")))          return tt_type_keyword_u64;
    if (string_eq(word, to_string("bool")))         return tt_type_keyword_bool;
    if (string_eq(word, to_string("float")))        return tt_type_keyword_float;
    if (string_eq(word, to_string("f16")))          return tt_type_keyword_f16;
    if (string_eq(word, to_string("f32")))          return tt_type_keyword_f32;
    if (string_eq(word, to_string("f64")))          return tt_type_keyword_f64;
    if (string_eq(word, to_string("addr")))         return tt_type_keyword_addr;

    if (string_eq(word, to_string("let")))          return tt_keyword_let;
    if (string_eq(word, to_string("mut")))          return tt_keyword_mut;
    if (string_eq(word, to_string("def")))          return tt_keyword_def;
    if (string_eq(word, to_string("type")))         return tt_keyword_type;
    if (string_eq(word, to_string("if")))           return tt_keyword_if;
    if (string_eq(word, to_string("elif")))         return tt_keyword_elif;
    if (string_eq(word, to_string("else")))         return tt_keyword_else;
    if (string_eq(word, to_string("for")))          return tt_keyword_for;
    if (string_eq(word, to_string("fn")))           return tt_keyword_fn;
    if (string_eq(word, to_string("asm")))          return tt_keyword_asm;
    if (string_eq(word, to_string("bitcast")))      return tt_keyword_bitcast;
    if (string_eq(word, to_string("break")))        return tt_keyword_break;
    if (string_eq(word, to_string("case")))         return tt_keyword_case;
    if (string_eq(word, to_string("cast")))         return tt_keyword_cast;
    if (string_eq(word, to_string("defer")))        return tt_keyword_defer;
    if (string_eq(word, to_string("enum")))         return tt_keyword_enum;
    if (string_eq(word, to_string("extern")))       return tt_keyword_extern;
    if (string_eq(word, to_string("fallthrough")))  return tt_keyword_fallthrough;
    if (string_eq(word, to_string("import")))       return tt_keyword_import;
    if (string_eq(word, to_string("module")))       return tt_keyword_module;
    if (string_eq(word, to_string("return")))       return tt_keyword_return;
    if (string_eq(word, to_string("struct")))       return tt_keyword_struct;
    if (string_eq(word, to_string("switch")))       return tt_keyword_switch;
    if (string_eq(word, to_string("union")))        return tt_keyword_union;
    if (string_eq(word, to_string("while")))        return tt_keyword_while;
    if (string_eq(word, to_string("typeof")))       return tt_keyword_typeof;
    if (string_eq(word, to_string("sizeof")))       return tt_keyword_sizeof;
    if (string_eq(word, to_string("alignof")))      return tt_keyword_alignof;
    if (string_eq(word, to_string("offsetof")))     return tt_keyword_offsetof;
    
    if (string_eq(word, to_string("true")))         return tt_literal_bool;
    if (string_eq(word, to_string("false")))        return tt_literal_bool;

    if (string_eq(word, to_string("null")))         return tt_literal_null;

    if (string_eq(word, to_string("_")))            return tt_identifier_discard;

    return tt_identifier;
}

token_type scan_number(lexer_state* restrict lex) {
    TODO("scan_number");
}
token_type scan_string_or_char(lexer_state* restrict lex) {
    TODO("scan_string_or_char");
}
token_type scan_operator(lexer_state* restrict lex) {
    // TODO("scan_operator");

    switch (current_char(lex)) {
    case '+':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_add_equal;
        }
        return tt_add;
    case '-':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_sub_equal;
        }
        if (current_char(lex) == '>') {
            advance_char(lex);
            return tt_arrow_right;
        }
        if (current_char(lex) == '-' && peek_char(lex, 1) == '-') {
            advance_char_n(lex, 2);
            return tt_uninit;
        }
        return tt_sub;
    case '*':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_mul_equal;
        }
        return tt_mul;
    case '/':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_div_equal;
        }
        return tt_div;
    case '%':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_mod_equal;
        }
        if (current_char(lex) == '%') {
            advance_char(lex);
                if (current_char(lex) == '=') {
                advance_char(lex);
                return tt_mod_mod_equal;
            }
            return tt_mod_mod;
        }
        return tt_mod;
    case '~':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_xor_equal;
        }
        if (current_char(lex) == '|') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return tt_nor_equal;
            }
            return tt_nor;
        }
        return tt_tilde;
    case '&':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_and_equal;
        }
        if (current_char(lex) == '&') {
            advance_char(lex);
            return tt_and_and;
        }
        return tt_and;
    case '|':
        advance_char(lex);
        if (current_char(lex) == '|') {
            advance_char(lex);
            return tt_or_equal;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_or_or;
        }
        return tt_or;
    case '<':
        advance_char(lex);
        if (current_char(lex) == '<') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return tt_lshift_equal;
            }
            return tt_lshift;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_less_equal;
        }
        return tt_less_than;
    case '>':
        advance_char(lex);
        if (current_char(lex) == '>') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return tt_rshift_equal;
            }
            return tt_rshift;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_greater_equal;
        }
        return tt_greater_than;
    case '=':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_equal_equal;
        }
        return tt_equal;
    case '!':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return tt_not_equal;
        }
        return tt_exclam;
    case ':':
        advance_char(lex);
        if (current_char(lex) == ':') {
            advance_char(lex);
            return tt_colon_colon;
        }
        return tt_colon;
    
    case '#': advance_char(lex); return tt_hash;
    case ';': advance_char(lex); return tt_hash;
    case '$': advance_char(lex); return tt_dollar;
    case '.': advance_char(lex); return tt_period;
    case ',': advance_char(lex); return tt_comma;
    case '^': advance_char(lex); return tt_carat;

    case '(': advance_char(lex); return tt_open_paren;
    case ')': advance_char(lex); return tt_close_paren;
    case '[': advance_char(lex); return tt_open_bracket;
    case ']': advance_char(lex); return tt_close_bracket;
    case '{': advance_char(lex); return tt_open_brace;
    case '}': advance_char(lex); return tt_close_brace;

    default:
        error_at_string(lex->path, lex->src, substring_len(lex->src, lex->cursor, 1), 
            "unrecognized character");
        break;
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
        if ((r != ' ' && r != '\t' && r != '\n' && r != '\r' && r != '\v') || lex->cursor >= lex->src.len) {
            return;
        }
        advance_char(lex);
    }
}