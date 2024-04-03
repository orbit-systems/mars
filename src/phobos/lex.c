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


int skip_block_comment(lexer* lex);
void skip_until_char(lexer* lex, char c);
void skip_whitespace(lexer* lex);

token_type scan_ident_or_keyword(lexer* lex);
token_type scan_number(lexer* lex);
token_type scan_string_or_char(lexer* lex);
token_type scan_operator(lexer* lex);

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

void construct_token_buffer(lexer* lex) {
    if (lex == NULL || is_null_str(lex->src) || is_null_str(lex->path))
        CRASH("bad lexer provided to construct_token_buffer");

    do {
        append_next_token(lex);
    } while (lex->buffer.at[lex->buffer.len-1].type != TOK_EOF);

    da_shrink(&lex->buffer);
}

void append_next_token(lexer* lex) {

    // if the top token is an EOF, early return
    // if (lex->buffer.at[lex->buffer.len-1].type == TOK_EOF) {
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
            ((token){substring_len(lex->src, lex->cursor, 1), TOK_EOF})
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

token_type scan_ident_or_keyword(lexer* lex) {
    u64 beginning = lex->cursor;
    
    advance_char(lex);
    while (can_be_in_identifier(current_char(lex))) advance_char(lex);

    string word = substring(lex->src, beginning, lex->cursor);

    if (string_eq(word, to_string("_")))            return TOK_IDENTIFIER_DISCARD;

    if (string_eq(word, to_string("int")))          return TOK_TYPE_KEYWORD_INT;
    if (string_eq(word, to_string("i8")))           return TOK_TYPE_KEYWORD_I8;
    if (string_eq(word, to_string("i16")))          return TOK_TYPE_KEYWORD_I16;
    if (string_eq(word, to_string("i32")))          return TOK_TYPE_KEYWORD_I32;
    if (string_eq(word, to_string("i64")))          return TOK_TYPE_KEYWORD_I64;
    if (string_eq(word, to_string("uint")))         return TOK_TYPE_KEYWORD_UINT;
    if (string_eq(word, to_string("u8")))           return TOK_TYPE_KEYWORD_U8;
    if (string_eq(word, to_string("u16")))          return TOK_TYPE_KEYWORD_U16;
    if (string_eq(word, to_string("u32")))          return TOK_TYPE_KEYWORD_U32;
    if (string_eq(word, to_string("u64")))          return TOK_TYPE_KEYWORD_U64;
    if (string_eq(word, to_string("bool")))         return TOK_TYPE_KEYWORD_BOOL;
    if (string_eq(word, to_string("float")))        return TOK_TYPE_KEYWORD_FLOAT;
    if (string_eq(word, to_string("f16")))          return TOK_TYPE_KEYWORD_F16;
    if (string_eq(word, to_string("f32")))          return TOK_TYPE_KEYWORD_F32;
    if (string_eq(word, to_string("f64")))          return TOK_TYPE_KEYWORD_F64;
    if (string_eq(word, to_string("addr")))         return TOK_TYPE_KEYWORD_ADDR;

    if (string_eq(word, to_string("let")))          return TOK_KEYWORD_LET;
    if (string_eq(word, to_string("mut")))          return TOK_KEYWORD_MUT;
    if (string_eq(word, to_string("def")))          return TOK_KEYWORD_DEF;
    if (string_eq(word, to_string("type")))         return TOK_KEYWORD_TYPE;
    if (string_eq(word, to_string("if")))           return TOK_KEYWORD_IF;
    if (string_eq(word, to_string("in")))           return TOK_KEYWORD_IN;
    if (string_eq(word, to_string("elif")))         return TOK_KEYWORD_ELIF;
    if (string_eq(word, to_string("else")))         return TOK_KEYWORD_ELSE;
    if (string_eq(word, to_string("for")))          return TOK_KEYWORD_FOR;
    if (string_eq(word, to_string("fn")))           return TOK_KEYWORD_FN;
    if (string_eq(word, to_string("break")))        return TOK_KEYWORD_BREAK;
    if (string_eq(word, to_string("continue")))     return TOK_KEYWORD_CONTINUE;
    if (string_eq(word, to_string("case")))         return TOK_KEYWORD_CASE;
    if (string_eq(word, to_string("cast")))         return TOK_KEYWORD_CAST;
    if (string_eq(word, to_string("defer")))        return TOK_KEYWORD_DEFER;
    if (string_eq(word, to_string("distinct")))     return TOK_KEYWORD_DISTINCT;
    if (string_eq(word, to_string("enum")))         return TOK_KEYWORD_ENUM;
    if (string_eq(word, to_string("extern")))       return TOK_KEYWORD_EXTERN;
    // if (string_eq(word, to_string("goto")))         return TOK_KEYWORD_GOTO;
    if (string_eq(word, to_string("asm")))          return TOK_KEYWORD_ASM;
    if (string_eq(word, to_string("bitcast")))      return TOK_KEYWORD_BITCAST;
    if (string_eq(word, to_string("import")))       return TOK_KEYWORD_IMPORT;
    if (string_eq(word, to_string("fallthrough")))  return TOK_KEYWORD_FALLTHROUGH;
    if (string_eq(word, to_string("module")))       return TOK_KEYWORD_MODULE;
    if (string_eq(word, to_string("return")))       return TOK_KEYWORD_RETURN;
    if (string_eq(word, to_string("struct")))       return TOK_KEYWORD_STRUCT;
    if (string_eq(word, to_string("switch")))       return TOK_KEYWORD_SWITCH;
    if (string_eq(word, to_string("union")))        return TOK_KEYWORD_UNION;
    if (string_eq(word, to_string("while")))        return TOK_KEYWORD_WHILE;
    if (string_eq(word, to_string("inline")))       return TOK_KEYWORD_INLINE;
    if (string_eq(word, to_string("sizeof")))       return TOK_KEYWORD_SIZEOF;
    if (string_eq(word, to_string("alignof")))      return TOK_KEYWORD_ALIGNOF;
    if (string_eq(word, to_string("offsetof")))     return TOK_KEYWORD_OFFSETOF;
    
    if (string_eq(word, to_string("true")))         return TOK_LITERAL_BOOL;
    if (string_eq(word, to_string("false")))        return TOK_LITERAL_BOOL;

    if (string_eq(word, to_string("null")))         return TOK_LITERAL_NULL;


    return TOK_IDENTIFIER;
}

token_type scan_number(lexer* lex) {
    advance_char(lex);
    while (true) {
        if (current_char(lex) == '.') {
            
            // really quick, check if its one of the range operators? this causes bugs very often :sob:
            if (peek_char(lex, 1) == '.') {
                return TOK_LITERAL_INT;
            }

            advance_char(lex);
            while (true) {
                if (current_char(lex) == 'e' && peek_char(lex, 1) == '-') {
                    advance_char_n(lex, 2);
                }
                if (!valid_digit(current_char(lex))) {
                    return TOK_LITERAL_FLOAT;
                }
                advance_char(lex);
            }
        }
        if (!valid_digit(current_char(lex))) {
            return TOK_LITERAL_INT;
        }
        advance_char(lex);
    }
}
token_type scan_string_or_char(lexer* lex) {
    char quote_char = current_char(lex);
    u64  start_cursor = lex->cursor;

    advance_char(lex);
    while (true) {
        if (current_char(lex) == '\\') {
            advance_char(lex);
        } else if (current_char(lex) == quote_char) {
            advance_char(lex);
            return quote_char == '\"' ? TOK_LITERAL_STRING : TOK_LITERAL_CHAR;
        } else if (current_char(lex) == '\n') {
            if (quote_char == '\"') error_at_string(lex->path, lex->src, substring(lex->src, start_cursor, lex->cursor),
                "unclosed string literal");
            if (quote_char == '\'') error_at_string(lex->path, lex->src, substring(lex->src, start_cursor, lex->cursor),
                "unclosed char literal");
        }
        advance_char(lex);
    }
}
token_type scan_operator(lexer* lex) {
    switch (current_char(lex)) {
    case '+':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_ADD_EQUAL;
        }
        return TOK_ADD;
    case '-':
        advance_char(lex);

        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_SUB_EQUAL;
        }
        if (current_char(lex) == '>') {
            advance_char(lex);
            return TOK_ARROW_RIGHT;
        }
        if (current_char(lex) == '-' && peek_char(lex, 1) == '-') {
            advance_char_n(lex, 2);
            return TOK_UNINIT;
        }
        return TOK_SUB;
    case '*':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_MUL_EQUAL;
        }
        return TOK_MUL;
    case '/':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_DIV_EQUAL;
        }
        return TOK_DIV;
    case '%':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_MOD_EQUAL;
        }
        if (current_char(lex) == '%') {
            advance_char(lex);
                if (current_char(lex) == '=') {
                advance_char(lex);
                return TOK_MOD_MOD_EQUAL;
            }
            return TOK_MOD_MOD;
        }
        return TOK_MOD;
    case '~':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_XOR_EQUAL;
        }
        if (current_char(lex) == '|') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return TOK_NOR_EQUAL;
            }
            return TOK_NOR;
        }
        return TOK_TILDE;
    case '&':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_AND_EQUAL;
        }
        if (current_char(lex) == '&') {
            advance_char(lex);
            return TOK_AND_AND;
        }
        return TOK_AND;
    case '|':
        advance_char(lex);
        if (current_char(lex) == '|') {
            advance_char(lex);
            return TOK_OR_OR;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_OR_EQUAL;
        }
        return TOK_OR;
    case '<':
        advance_char(lex);
        if (current_char(lex) == '<') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return TOK_LSHIFT_EQUAL;
            }
            return TOK_LSHIFT;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_LESS_EQUAL;
        }
        return TOK_LESS_THAN;
    case '>':
        advance_char(lex);
        if (current_char(lex) == '>') {
            advance_char(lex);
            if (current_char(lex) == '=') {
                advance_char(lex);
                return TOK_RSHIFT_EQUAL;
            }
            return TOK_RSHIFT;
        }
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_GREATER_EQUAL;
        }
        return TOK_GREATER_THAN;
    case '=':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_EQUAL_EQUAL;
        }
        return TOK_EQUAL;
    case '!':
        advance_char(lex);
        if (current_char(lex) == '=') {
            advance_char(lex);
            return TOK_NOT_EQUAL;
        }
        return TOK_EXCLAM;
    case ':':
        advance_char(lex);
        if (current_char(lex) == ':') {
            advance_char(lex);
            return TOK_COLON_COLON;
        }
        return TOK_COLON;
    case '.':
        advance_char(lex);
        if (current_char(lex) == '.') {
            if (peek_char(lex, 1) == '<') {
                advance_char_n(lex, 2);
                return TOK_RANGE_LESS;
            }
            if (peek_char(lex, 1) == '=') {
                advance_char_n(lex, 2);
                return TOK_RANGE_EQ;
            }
        } else if (valid_0d(current_char(lex))) {
            advance_char_n(lex, -2);
            return scan_number(lex);
        }
        return TOK_PERIOD;

    case '#': advance_char(lex); return TOK_HASH;
    case ';': advance_char(lex); return TOK_SEMICOLON;
    case '$': advance_char(lex); return TOK_DOLLAR;
    case ',': advance_char(lex); return TOK_COMMA;
    case '^': advance_char(lex); return TOK_CARET;
    case '@': advance_char(lex); return TOK_AT;

    case '(': advance_char(lex); return TOK_OPEN_PAREN;
    case ')': advance_char(lex); return TOK_CLOSE_PAREN;
    case '[': advance_char(lex); return TOK_OPEN_BRACKET;
    case ']': advance_char(lex); return TOK_CLOSE_BRACKET;
    case '{': advance_char(lex); return TOK_OPEN_BRACE;
    case '}': advance_char(lex); return TOK_CLOSE_BRACE;

    default:
        error_at_string(lex->path, lex->src, substring_len(lex->src, lex->cursor, 1), 
            "unrecognized character");
        break;
    }
    return TOK_INVALID;
}

int skip_block_comment(lexer* lex) {
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

void skip_until_char(lexer* lex, char c) {
    while (current_char(lex) != c && lex->cursor < lex->src.len) {
        advance_char(lex);
    }
}

void skip_whitespace(lexer* lex) {
    while (true) {
        char r = current_char(lex);
        if ((r != ' ' && r != '\t' && r != '\n' && r != '\r' && r != '\v') || lex->cursor >= lex->src.len) {
            return;
        }
        advance_char(lex);
    }
}