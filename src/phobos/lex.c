#include "orbit.h"
#include "term.h"
#include "lex.h"

#define can_start_identifier(ch) ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_')
#define can_be_in_identifier(ch) ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')
#define can_start_number(ch) ((ch >= '0' && ch <= '9'))
#define valid_digit(ch) ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_'))

#define valid_0x(ch) ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') || (ch == '_'))
#define valid_0d(ch) ((ch >= '0' && ch <= '9') || (ch == '_'))
#define valid_0o(ch) ((ch >= '0' && ch <= '7') || (ch == '_'))
#define valid_0b(ch) (ch == '0' || ch == '1' || ch == '_')

#define current_char(lex) (lex->current_char)
#define advance_char(lex) (lex->cursor < lex->src.len ? (lex->current_char = lex->src.raw[++lex->cursor]) : '\0')
#define advance_char_n(lex, n) (lex->cursor + (n) < lex->src.len ? (lex->current_char = lex->src.raw[lex->cursor += (n)]) : '\0')
#define peek_char(lex, n) ((lex->cursor + (n)) < lex->src.len ? lex->src.raw[lex->cursor + (n)] : '\0')


int skip_block_comment(lexer* restrict lex);
void skip_until_char(lexer* restrict lex, char c);
void skip_whitespace(lexer* restrict lex);

token_type scan_ident_or_keyword(lexer* restrict lex);
token_type scan_number(lexer* restrict lex);
token_type scan_string_or_char(lexer* restrict lex);
token_type scan_operator(lexer* restrict lex);

char* token_type_str[] = {
#define TOKEN(enum, str) str,
    TOKEN_LIST
#undef TOKEN
};

lexer new_lexer(string path, string src) {
    lexer lex = {0};
    lex.path = path;
    lex.src = src;
    lex.current_char = src.raw[0];
    lex.cursor = 0;
    da_init(&lex.buffer, src.len/3.5);
    return lex;
}

void construct_token_buffer(lexer* restrict lex) {
    if (lex == NULL || is_null_str(lex->src) || is_null_str(lex->path))
        CRASH("bad lexer provided to construct_token_buffer");

    do {
        append_next_token(lex);
    } while (lex->buffer.at[lex->buffer.len-1].type != TT_EOF);

    da_shrink(&lex->buffer);
}

void append_next_token(lexer* restrict lex) {

    // if the top token is an EOF, early return
    // if (lex->buffer.at[lex->buffer.len-1].type == TT_EOF) {
    //     return;
    // }

    // advance to next significant char
    while (true) {
        if (lex->cursor >= lex->src.len) goto skip_insignificant_end;
        switch (current_char(lex)) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\v':
            advance_char(lex);
            continue;
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
            continue;
        default:
            goto skip_insignificant_end;
        }
    }
    skip_insignificant_end:

    if (lex->cursor >= lex->src.len) {
        da_append(
            &lex->buffer, 
            ((token){substring_len(lex->src, lex->cursor, 1), tt_EOF})
            ((token){substring_len(lex->src, lex->cursor, 1), TT_EOF})
        );
        return;
    }

    u64 beginning_cursor = lex->cursor;
    token_type this_type;
    if (can_start_identifier(current_char(lex))) {
        this_type = scan_ident_or_keyword(lex);
    } else if (can_start_number(current_char(lex))) {
        this_type = scan_number(lex);
    } else if (current_char(lex) == '\"' || current_char(lex) == '\'') {
        this_type = scan_string_or_char(lex);
    } else {
        this_type = scan_operator(lex);
    }

    da_append(&lex->buffer, ((token){
        .text = substring(lex->src, beginning_cursor, lex->cursor), 
        .type = this_type,
    }));
}

token_type scan_ident_or_keyword(lexer* restrict lex) {
    u64 beginning = lex->cursor;
    
    advance_char(lex);
    while (can_be_in_identifier(current_char(lex))) advance_char(lex);

    string word = substring(lex->src, beginning, lex->cursor);

    if (string_eq(word, to_string("_")))            return TT_IDENTIFIER_DISCARD;

    if (string_eq(word, to_string("int")))          return TT_TYPE_KEYWORD_INT;
    if (string_eq(word, to_string("i8")))           return TT_TYPE_KEYWORD_I8;
    if (string_eq(word, to_string("i16")))          return TT_TYPE_KEYWORD_I16;
    if (string_eq(word, to_string("i32")))          return TT_TYPE_KEYWORD_I32;
    if (string_eq(word, to_string("i64")))          return TT_TYPE_KEYWORD_I64;
    if (string_eq(word, to_string("uint")))         return TT_TYPE_KEYWORD_UINT;
    if (string_eq(word, to_string("u8")))           return TT_TYPE_KEYWORD_U8;
    if (string_eq(word, to_string("u16")))          return TT_TYPE_KEYWORD_U16;
    if (string_eq(word, to_string("u32")))          return TT_TYPE_KEYWORD_U32;
    if (string_eq(word, to_string("u64")))          return TT_TYPE_KEYWORD_U64;
    if (string_eq(word, to_string("bool")))         return TT_TYPE_KEYWORD_BOOL;
    if (string_eq(word, to_string("float")))        return TT_TYPE_KEYWORD_FLOAT;
    if (string_eq(word, to_string("f16")))          return TT_TYPE_KEYWORD_F16;
    if (string_eq(word, to_string("f32")))          return TT_TYPE_KEYWORD_F32;
    if (string_eq(word, to_string("f64")))          return TT_TYPE_KEYWORD_F64;
    if (string_eq(word, to_string("addr")))         return TT_TYPE_KEYWORD_ADDR;

    if (string_eq(word, to_string("let")))          return TT_KEYWORD_LET;
    if (string_eq(word, to_string("mut")))          return TT_KEYWORD_MUT;
    if (string_eq(word, to_string("def")))          return TT_KEYWORD_DEF;
    if (string_eq(word, to_string("type")))         return TT_KEYWORD_TYPE;
    if (string_eq(word, to_string("if")))           return TT_KEYWORD_IF;
    if (string_eq(word, to_string("in")))           return TT_KEYWORD_IN;
    if (string_eq(word, to_string("elif")))         return TT_KEYWORD_ELIF;
    if (string_eq(word, to_string("else")))         return TT_KEYWORD_ELSE;
    if (string_eq(word, to_string("for")))          return TT_KEYWORD_FOR;
    if (string_eq(word, to_string("fn")))           return TT_KEYWORD_FN;
    if (string_eq(word, to_string("break")))        return TT_KEYWORD_BREAK;
    if (string_eq(word, to_string("continue")))     return TT_KEYWORD_CONTINUE;
    if (string_eq(word, to_string("case")))         return TT_KEYWORD_CASE;
    if (string_eq(word, to_string("cast")))         return TT_KEYWORD_CAST;
    if (string_eq(word, to_string("defer")))        return TT_KEYWORD_DEFER;
    if (string_eq(word, to_string("distinct")))     return TT_KEYWORD_DISTINCT;
    if (string_eq(word, to_string("enum")))         return TT_KEYWORD_ENUM;
    if (string_eq(word, to_string("extern")))       return TT_KEYWORD_EXTERN;
    // if (string_eq(word, to_string("goto")))         return TT_KEYWORD_GOTO;
    if (string_eq(word, to_string("asm")))          return TT_KEYWORD_ASM;
    if (string_eq(word, to_string("bitcast")))      return TT_KEYWORD_BITCAST;
    if (string_eq(word, to_string("import")))       return TT_KEYWORD_IMPORT;
    if (string_eq(word, to_string("fallthrough")))  return TT_KEYWORD_FALLTHROUGH;
    if (string_eq(word, to_string("module")))       return TT_KEYWORD_MODULE;
    if (string_eq(word, to_string("return")))       return TT_KEYWORD_RETURN;
    if (string_eq(word, to_string("struct")))       return TT_KEYWORD_STRUCT;
    if (string_eq(word, to_string("switch")))       return TT_KEYWORD_SWITCH;
    if (string_eq(word, to_string("union")))        return TT_KEYWORD_UNION;
    if (string_eq(word, to_string("while")))        return TT_KEYWORD_WHILE;
    if (string_eq(word, to_string("inline")))       return TT_KEYWORD_INLINE;
    if (string_eq(word, to_string("sizeof")))       return TT_KEYWORD_SIZEOF;
    if (string_eq(word, to_string("alignof")))      return TT_KEYWORD_ALIGNOF;
    if (string_eq(word, to_string("offsetof")))     return TT_KEYWORD_OFFSETOF;
    
    if (string_eq(word, to_string("true")))         return TT_LITERAL_BOOL;
    if (string_eq(word, to_string("false")))        return TT_LITERAL_BOOL;

    if (string_eq(word, to_string("null")))         return TT_LITERAL_NULL;


    return TT_IDENTIFIER;
}

token_type scan_number(lexer* restrict lex) {
    advance_char(lex);
    while (true) {
        if (current_char(lex) == '.') {
            
            // really quick, check if its one of the range operators? this causes bugs very often :sob:
            if (peek_char(lex, 1) == '.') {
                return TT_LITERAL_INT;
            }

            advance_char(lex);
            while (true) {
                if (current_char(lex) == 'e' && peek_char(lex, 1) == '-') {
                    advance_char_n(lex, 2);
                }
                if (!valid_digit(current_char(lex))) {
                    return TT_LITERAL_FLOAT;
                }
                advance_char(lex);
            }
        }
        if (!valid_digit(current_char(lex))) {
            return TT_LITERAL_INT;
        }
        advance_char(lex);
    }
}
token_type scan_string_or_char(lexer* restrict lex) {
    char quote_char = current_char(lex);
    u64  start_cursor = lex->cursor;

    advance_char(lex);
    while (true) {
        if (current_char(lex) == '\\') {
            advance_char(lex);
        } else if (current_char(lex) == quote_char) {
            advance_char(lex);
            return quote_char == '\"' ? TT_LITERAL_STRING : TT_LITERAL_CHAR;
        } else if (current_char(lex) == '\n') {
            if (quote_char == '\"') error_at_string(lex->path, lex->src, substring(lex->src, start_cursor, lex->cursor),
                "unclosed string literal");
            if (quote_char == '\'') error_at_string(lex->path, lex->src, substring(lex->src, start_cursor, lex->cursor),
                "unclosed char literal");
        }
        advance_char(lex);
    }
}
token_type scan_operator(lexer* restrict lex) {
    switch (current_char(lex)) {
    case '+':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_ADD_EQUAL;
        }
        return TT_ADD;
    case '-':
        advance_char(lex);

        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_SUB_EQUAL;
        }
        if (current_char(lex) == '>') {
            advance_char(lex);
            return TT_ARROW_RIGHT;
        }
        if (current_char(lex) == '-' && peek_char(lex, 1) == '-') {
            advance_char_n(lex, 2);
            return TT_UNINIT;
        }
        return TT_SUB;
    case '*':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_MUL_EQUAL;
        }
        return TT_MUL;
    case '/':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_DIV_EQUAL;
        }
        return TT_DIV;
    case '%':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_MOD_EQUAL;
        }
        if (current_char(lex) == '%') {
            advance_char(lex);
                if (current_char(lex) == '=') {
                advance_char(lex);
                return TT_MOD_MOD_EQUAL;
            }
            return TT_MOD_MOD;
        }
        return TT_MOD;
    case '~':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_XOR_EQUAL;
        }
        if (current_char(lex) == '|') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return TT_NOR_EQUAL;
            }
            return TT_NOR;
        }
        return TT_TILDE;
    case '&':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_AND_EQUAL;
        }
        if (current_char(lex) == '&') {
            advance_char(lex);
            return TT_AND_AND;
        }
        return TT_AND;
    case '|':
        advance_char(lex);
        if (current_char(lex) == '|') {
            advance_char(lex);
            return TT_OR_OR;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_OR_EQUAL;
        }
        return TT_OR;
    case '<':
        advance_char(lex);
        if (current_char(lex) == '<') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return TT_LSHIFT_EQUAL;
            }
            return TT_LSHIFT;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_LESS_EQUAL;
        }
        return TT_LESS_THAN;
    case '>':
        advance_char(lex);
        if (current_char(lex) == '>') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return TT_RSHIFT_EQUAL;
            }
            return TT_RSHIFT;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_GREATER_EQUAL;
        }
        return TT_GREATER_THAN;
    case '=':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_EQUAL_EQUAL;
        }
        return TT_EQUAL;
    case '!':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TT_NOT_EQUAL;
        }
        return TT_EXCLAM;
    case ':':
        advance_char(lex);
        if (current_char(lex) == ':') {
            advance_char(lex);
            return TT_COLON_COLON;
        }
        return TT_COLON;
    case '.':
        advance_char(lex);
        if (current_char(lex) == '.') {
            if (peek_char(lex, 1) == '<') {
                advance_char_n(lex, 2);
                return TT_RANGE_LESS;
            }
            if (peek_char(lex, 1) == '=') {
                advance_char_n(lex, 2);
                return TT_RANGE_EQ;
            }
        } else if (valid_0d(current_char(lex))) {
            advance_char_n(lex, -2);
            return scan_number(lex);
        }
        return TT_PERIOD;

    case '#': advance_char(lex); return TT_HASH;
    case ';': advance_char(lex); return TT_SEMICOLON;
    case '$': advance_char(lex); return TT_DOLLAR;
    case ',': advance_char(lex); return TT_COMMA;
    case '^': advance_char(lex); return TT_CARAT;
    case '@': advance_char(lex); return TT_AT;

    case '(': advance_char(lex); return TT_OPEN_PAREN;
    case ')': advance_char(lex); return TT_CLOSE_PAREN;
    case '[': advance_char(lex); return TT_OPEN_BRACKET;
    case ']': advance_char(lex); return TT_CLOSE_BRACKET;
    case '{': advance_char(lex); return TT_OPEN_BRACE;
    case '}': advance_char(lex); return TT_CLOSE_BRACE;

    default:
        error_at_string(lex->path, lex->src, substring_len(lex->src, lex->cursor, 1), 
            "unrecognized character");
        break;
    }
    return TT_INVALID;
}

int skip_block_comment(lexer* restrict lex) {
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

void skip_until_char(lexer* restrict lex, char c) {
    while (current_char(lex) != c && lex->cursor < lex->src.len) {
        advance_char(lex);
    }
}

void skip_whitespace(lexer* restrict lex) {
    while (true) {
        char r = current_char(lex);
        if ((r != ' ' && r != '\t' && r != '\n' && r != '\r' && r != '\v') || lex->cursor >= lex->src.len) {
            return;
        }
        advance_char(lex);
    }
}