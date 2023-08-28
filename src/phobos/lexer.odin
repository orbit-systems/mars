package phobos

import "core:fmt"
import "core:os"
import "core:unicode/utf8"

lex_next_token :: proc(ctx: ^lexer_info) -> (this_token: lexer_token) {

    if ctx == nil {
        fmt.printf("Nil context provided to lex_next_token.\n")
        return //return empty lexer token
    }

    if ctx.file_data == "" || ctx.file_name == "" {
        fmt.printf("oopsie poopsie no lexing for you! you didn't give me any information! stupid little shit\n")
        os.exit(-1)
    }

    // initalize rows and cols to 1 if not set
    ctx.current_col = (ctx.current_col == 0) ? 1 : ctx.current_col
    ctx.current_row = (ctx.current_row == 0) ? 1 : ctx.current_row

    cursor_rune : rune
    
    // advance to next significant rune
    loop: for {
        cursor_rune = lex_current_rune(ctx)
        switch cursor_rune {
        case ' ', '\t', '\n', '\r':
            cursor_rune, hit_eof := skip_whitespace(ctx)
            if hit_eof {
                this_token = make_EOF(ctx)
                break loop
            }
            continue
        case '/':
            // determine what kind of comment we're dealing with here
            if lex_peek_next_rune(ctx) == '/' {
                cursor_rune, hit_eof := skip_line_comment(ctx)
                if hit_eof {
                    this_token = make_EOF(ctx)
                    break loop
                }
            } else if lex_peek_next_rune(ctx) == '*' {
                lex_advance_cursor(ctx)
                lex_advance_cursor(ctx)
                cursor_rune, hit_eof, block_comment_end_level := skip_block_comment(ctx)
                if block_comment_end_level > 0 {
                    fmt.printf("ERROR [ %s :: %d : %d ] unclosed block comment\n", ctx.file_name, ctx.current_row, ctx.current_col)
                    os.exit(0)
                }
                if hit_eof {
                    this_token = make_EOF(ctx)
                    break loop
                }
            } else {
                // nevermind its an operator
                break loop
            }
        case:
            break loop
        }
    }
    cursor_rune = lex_current_rune(ctx)

    // reset offset
    lexeme_start_pos := ctx.current_offset


    // scan actual token
    switch cursor_rune {
    case '0'..='9':
        success := scan_number(ctx)
        fmt.printf("ERROR [ %s :: %d : %d ] invalid literal \"%s\"", ctx.file_name, ctx.current_row, ctx.current_col,
            ctx.file_data[lexeme_start_pos:ctx.current_offset])
        os.exit(0)
    case 'A'..='Z', 'a'..='z', '_':
        success := scan_identifier(ctx)
        fmt.printf("ERROR [ %s :: %d : %d ] invalid identifier \"%s\"", ctx.file_name, ctx.current_row, ctx.current_col,
            ctx.file_data[lexeme_start_pos:ctx.current_offset])
        os.exit(0)
    case '\"':
        success := scan_string_literal(ctx)
        fmt.printf("ERROR [ %s :: %d : %d ] invalid string literal \"%s\"", ctx.file_name, ctx.current_row, ctx.current_col,
            ctx.file_data[lexeme_start_pos:ctx.current_offset])
        os.exit(0)
    case:
        success := scan_operator(ctx)
        fmt.printf("ERROR [ %s :: %d : %d ] invalid operator \"%s\"", ctx.file_name, ctx.current_row, ctx.current_col,
            ctx.file_data[lexeme_start_pos:ctx.current_offset])
        os.exit(0)
    }

    
    return
}

// TODO implement
scan_number :: proc(ctx: ^lexer_info) -> (success: token_kind) {
    return
}

// TODO implement
scan_identifier :: proc(ctx: ^lexer_info) -> (success: token_kind) {
    return
}

// TODO implement
scan_string_literal :: proc(ctx: ^lexer_info) -> (success: token_kind) {
    return
}

// TODO implement
scan_operator :: proc(ctx: ^lexer_info) -> (success: token_kind) {
    return
}

skip_line_comment :: #force_inline proc(ctx: ^lexer_info) -> (r: rune, hit_eof: bool) {
    return skip_until_rune(ctx, '\n')
}

skip_block_comment :: proc(ctx: ^lexer_info) -> (r: rune, hit_eof: bool, level: int) {
    level = 1
    for level != 0 {
        r = lex_current_rune(ctx)
        if ctx.current_offset >= len(ctx.file_data) {
            return r, true, level
        }
        if r == '/' && lex_peek_next_rune(ctx) == '*' {
            lex_advance_cursor(ctx)
            level += 1
        }
        else if r == '*' && lex_peek_next_rune(ctx) == '/' {
            lex_advance_cursor(ctx)
            level -= 1
        }
        lex_advance_cursor(ctx)
    }
    return r, false, level
}

skip_whitespace :: proc(ctx: ^lexer_info) -> (r: rune, hit_eof: bool) {
    for {
        r = lex_current_rune(ctx)
        if ctx.current_offset >= len(ctx.file_data) {
            return r, true
        }
        if !is_whitespace(r) {
            return r, false
        }
        lex_advance_cursor(ctx)
    }
}

skip_until_rune :: proc(ctx: ^lexer_info, lookout: rune) -> (r: rune, hit_eof: bool) {
    for {
        r = lex_current_rune(ctx)
        if ctx.current_offset >= len(ctx.file_data) {
            return r, true
        }
        if r == lookout {
            return r, false
        }
        lex_advance_cursor(ctx)
    }
}

// b r u h                            b r u h
//   ^- current rune: 'r'     ->          ^- current rune: 'u'
// return current rune, move cursor forward
lex_advance_cursor :: proc(ctx: ^lexer_info) -> (r: rune, byte_len: int) {
    r, byte_len = lex_current_rune_and_len(ctx)
    ctx.current_offset += uint(byte_len)
    ctx.current_col += 1
    if r == '\n' {
        ctx.current_col = 0
        ctx.current_row += 1
    }
    return
}

lex_peek_next_rune :: proc(ctx: ^lexer_info) -> (r: rune) {
    _, byte_offset := lex_current_rune_and_len(ctx)
    return utf8.rune_at(ctx.file_data, int(ctx.current_offset)+byte_offset)
}

// b r u h                             b r u h
//   ^- current rune: 'r'      ->        ^- current rune: 'r'
// return current rune, keep cursor
lex_current_rune :: #force_inline proc(ctx: ^lexer_info) -> (r: rune) {
    r, _ = lex_current_rune_and_len(ctx)
    return
}
lex_current_rune_and_len :: #force_inline proc(ctx: ^lexer_info) -> (r: rune, byte_len: int) {
    // all because utf8.rune_at discards the damn byte length. fuck you utf8.rune_at. 
    return utf8.decode_rune_in_string(ctx.file_data[ctx.current_offset:])
}

make_EOF :: #force_inline proc(ctx: ^lexer_info) -> lexer_token {
    return lexer_token{.EOF,"",make_position(ctx)}
}

make_position :: #force_inline proc(ctx: ^lexer_info) -> position {
    return position{
        ctx.file_name,
        ctx.current_row,
        ctx.current_col,
    }
}

whitespace_runes :: [?]rune{' ', '\t', '\n', '\r'}
is_whitespace :: proc(r: rune) -> bool {
    for whitespace in whitespace_runes {
        if r == whitespace {
            return true
        }
    }
    return false
}