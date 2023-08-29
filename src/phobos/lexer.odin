package phobos

import "core:fmt"
import "core:os"
import "core:unicode/utf8"
import "core:strconv"

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


    // reset offset
    ctx.start_offset = ctx.current_offset
    start_row := ctx.current_row
    start_col := ctx.current_col

    cursor_rune, _ = lex_advance_cursor(ctx)

    // scan actual token
    switch cursor_rune {
    case '0'..='9':
        success := scan_number(ctx, cursor_rune)
        if success == .invalid {
            fmt.printf("ERROR [ %s :: %d : %d ] invalid numeric literal \"%s\"", ctx.file_name, start_row, start_col,
                lex_get_current_substring(ctx))
            os.exit(0)
        } else {
            this_token.kind = success
        }
    case 'A'..='Z', 'a'..='z', '_':
        success := scan_identifier(ctx, cursor_rune)
        if success == .invalid {
            fmt.printf("ERROR [ %s :: %d : %d ] invalid identifier \"%s\"", ctx.file_name, start_row, start_col,
                lex_get_current_substring(ctx))
            os.exit(0)
        } else {
            this_token.kind = success
        }
    case '\"':
        success := scan_string_literal(ctx, cursor_rune)
        if success == .invalid {
            fmt.printf("ERROR [ %s :: %d : %d ] invalid string literal \"%s\"", ctx.file_name, start_row, start_col,
                lex_get_current_substring(ctx))
            os.exit(0)
        } else {
            this_token.kind = success
        }
    case:
        success := scan_operator(ctx, cursor_rune)
        if success == .invalid {
            fmt.printf("ERROR [ %s :: %d : %d ] invalid operator \"%s\"", ctx.file_name, start_row, start_col,
                lex_get_current_substring(ctx))
            os.exit(0)
        } else {
            this_token.kind = success
        }
    }

    fmt.println(lex_get_current_substring(ctx), this_token.kind)
    this_token.lexeme = lex_get_current_substring(ctx)
    
    return
}

// ! this is jank and will cause problems later maybe!
scan_number :: proc(ctx: ^lexer_info, r: rune) -> (success: token_kind) {
    loop: for {
        switch lex_current_rune(ctx) {
        case '0'..='9', '.', 'x', 'b', 'o':
            lex_advance_cursor(ctx)
        case:
            _, i64_ok := strconv.parse_i64(lex_get_current_substring(ctx))
            if i64_ok {
                return .literal_int
            }
            _, float_ok := strconv.parse_f64(lex_get_current_substring(ctx))
            if float_ok {
                return .literal_float
            }
            break loop
        }
    }

    return
}

// TODO implement
scan_identifier :: proc(ctx: ^lexer_info, r: rune) -> (success: token_kind) {
    loop: for {
        switch lex_current_rune(ctx) {
        case 'A'..='Z', 'a'..='z', '_', '0'..='9':
            lex_advance_cursor(ctx)
        case:
            break loop
        }
    }

    switch lex_get_current_substring(ctx){
    case "asm":             return .keyword_asm
    case "bitcast":         return .keyword_bitcast
    case "break":           return .keyword_break
    case "case":            return .keyword_case
    case "cast":            return .keyword_cast
    case "defer":           return .keyword_defer
    case "enum":            return .keyword_enum
    case "elif":            return .keyword_elif
    case "else":            return .keyword_else
    case "external":        return .keyword_external
    case "fallthrough":     return .keyword_fallthrough
    case "for":             return .keyword_for
    case "fn":              return .keyword_fn
    case "if":              return .keyword_if
    case "import":          return .keyword_import
    case "module":          return .keyword_module
    case "return":          return .keyword_return
    case "sizeof":          return .keyword_sizeof
    case "struct":          return .keyword_struct
    case "switch":          return .keyword_switch
    case "union":           return .keyword_union
    case "while":           return .keyword_while
    case "true", "false":   return .literal_bool
    case "null":            return .literal_null
    case "_":               return .identifier_discard
    case:                   return .identifier
    }

    return
}

// TODO implement
scan_string_literal :: proc(ctx: ^lexer_info, r: rune) -> (success: token_kind) {
    return
}

// TODO implement
scan_operator :: proc(ctx: ^lexer_info, r: rune) -> (success: token_kind) {
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

lex_get_current_substring :: #force_inline proc(ctx: ^lexer_info) -> string {
    return ctx.file_data[ctx.start_offset:ctx.current_offset]
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
        ctx.current_offset,
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