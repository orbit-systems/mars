package phobos

import co "../common"
import "core:fmt"
import "core:os"
import "core:strconv"

construct_token_buffer :: proc(ctx: ^lexer) {

    int_token : lexer_token
    for int_token.kind != .EOF {
        int_token = lex_next_token(ctx)
        append(&ctx.buffer, int_token)
    }

    shrink(&ctx.buffer)
}

lex_next_token :: proc(ctx: ^lexer) -> (this_token: lexer_token) {
    if ctx == nil {
        fmt.printf("Nil context provided to lex_next_token\n")
        return //return empty lexer token
    }

    if ctx.src == "" || ctx.path == "" {
        fmt.printf("CRASH: oopsie poopsie no lexing for you! stupid little shit (contact me)\n")
        os.exit(-1)
    }

    cursor_rune : u8
    
    // advance to next significant rune
    loop: for {
        if ctx.pos.offset >= len(ctx.src) {
            return make_EOF(ctx)
        }
        cursor_rune = lex_current_char(ctx)
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
            if lex_peek_next_char(ctx) == '/' {
                cursor_rune, hit_eof := skip_line_comment(ctx)
                if hit_eof {
                    this_token = make_EOF(ctx)
                    break loop
                }
            } else if lex_peek_next_char(ctx) == '*' {
                lex_advance_cursor(ctx)
                lex_advance_cursor(ctx)
                cursor_rune, hit_eof, block_comment_end_level := skip_block_comment(ctx)
                if block_comment_end_level > 0 {
                    error(ctx.path, ctx.src, ctx.pos, "unclosed block comment", no_print_line = true)
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
    ctx.pos.start = ctx.pos.offset
    start_row := ctx.pos.line
    start_col := ctx.pos.col

    if ctx.pos.offset >= len(ctx.src) {
        return make_EOF(ctx)
    }
    cursor_rune = lex_advance_cursor(ctx)

    // scan actual token
    if ctx.pos.offset >= len(ctx.src) {
        return make_EOF(ctx)
    }
    switch cursor_rune {
    case '0'..='9':
        success := scan_number(ctx, cursor_rune)
        if success == .invalid {
            error(ctx.path, ctx.src, ctx.pos, "invalid numeric literal \"%s\"", get_substring(ctx.src, ctx.pos))
        } else {
            this_token.kind = success
        }
    case 'A'..='Z', 'a'..='z', '_':
        success := scan_identifier(ctx, cursor_rune)
        if success == .invalid {
            error(ctx.path, ctx.src, ctx.pos, "invalid identifier \"%s\"", get_substring(ctx.src, ctx.pos))
        } else {
            this_token.kind = success
        }
    case '\"':
        success := scan_string_literal(ctx, cursor_rune)
        if success == .invalid {
            error(ctx.path, ctx.src, ctx.pos, "invalid string literal \"%s\"", get_substring(ctx.src, ctx.pos))
        } else {
            this_token.kind = success
        }
    case:
        success := scan_operator(ctx, cursor_rune)
        if success == .invalid {
            if get_substring(ctx.src, ctx.pos)[0] == '\'' {
                error(ctx.path, ctx.src, ctx.pos, "single quotes not supported")

            }                 
            error(ctx.path, ctx.src, ctx.pos, "invalid operator \"%s\"", get_substring(ctx.src, ctx.pos))
        } else {
            this_token.kind = success
        }
    }

    // * this is significantly slow for some reason!! it slows lexing down by like .3 seconds in the stresstest
    //this_token.lexeme = get_substring(ctx.src, ctx.pos)
    
    this_token.pos = ctx.pos
    
    

    return
}

// TODO this is jank, please reimplement better
scan_number :: proc(ctx: ^lexer, r: u8) -> (success: token_kind) {
    loop: for {
        switch lex_current_char(ctx) {
        case '0'..='9', '.', 'x', 'b', 'o':
            lex_advance_cursor(ctx)
        case:
            _, i64_ok := strconv.parse_i64(get_substring(ctx.src, ctx.pos))
            if i64_ok {
                return .literal_int
            }
            _, float_ok := strconv.parse_f64(get_substring(ctx.src, ctx.pos))
            if float_ok {
                return .literal_float
            }
            break loop
        }
    }

    return
}

scan_identifier :: proc(ctx: ^lexer, r: u8) -> (success: token_kind) {
    loop: for {
        switch lex_current_char(ctx) {
        case 'A'..='Z', 'a'..='z', '_', '0'..='9':
            lex_advance_cursor(ctx)
        case:
            break loop
        }
    }

    switch get_substring(ctx.src, ctx.pos) {
    case "asm":           return .keyword_asm
    case "bitcast":       return .keyword_bitcast
    case "break":         return .keyword_break
    case "case":          return .keyword_case
    case "cast":          return .keyword_cast
    case "defer":         return .keyword_defer
    case "enum":          return .keyword_enum
    case "elif":          return .keyword_elif
    case "else":          return .keyword_else
    case "external":      return .keyword_external
    case "fallthrough":   return .keyword_fallthrough
    case "for":           return .keyword_for
    case "fn":            return .keyword_fn
    case "if":            return .keyword_if
    case "import":        return .keyword_import
    case "module":        return .keyword_module
    case "return":        return .keyword_return
    case "sizeof":        return .keyword_sizeof
    case "struct":        return .keyword_struct
    case "switch":        return .keyword_switch
    case "union":         return .keyword_union
    case "while":         return .keyword_while
    case "true", "false": return .literal_bool
    case "null":          return .literal_null
    case "_":             return .identifier_discard
    case:                 return .identifier
    }

    return
}

scan_string_literal :: proc(ctx: ^lexer, r: u8) -> (success: token_kind) {
    for {
        switch lex_current_char(ctx) {
        case '\"':
            lex_advance_cursor(ctx)
            return .literal_string
        case '\\':
            lex_advance_cursor(ctx)
            lex_advance_cursor(ctx)
        case '\n':
            error(ctx.path, ctx.src, ctx.pos, "string not closed")
        case:
            lex_advance_cursor(ctx)
        }
    }
    return
}

scan_operator :: proc(ctx: ^lexer, r: u8) -> (success: token_kind) {
    this_rune := r
    switch this_rune {
    case '#':       // #
        return .hash
    case '-':       // -
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '-':   // --
            lex_advance_cursor(ctx)
            this_rune = lex_current_char(ctx)
            switch this_rune {
            case '-': // ---
                lex_advance_cursor(ctx)
                return .uninit
            case:
                return .invalid
            }
        case '>':   // ->
            lex_advance_cursor(ctx)
            return .arrow_right
        case '=':   // -=
            lex_advance_cursor(ctx)
            return .sub_equal
        case:       // -
            return .sub
        }
    case '=':       // =
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '=':   // ==
            lex_advance_cursor(ctx)
            return .equal_equal
        case:       // =
            return .equal
        }
    case '$':       // $
        return .dollar
    case ':':       // :
        return .colon
    case ';':       // ;
        return .semicolon
    case '.':       // .
        return .period
    case ',':       // ,
        return .comma
    case '!':       // !
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '=':   // !=
            lex_advance_cursor(ctx)
            return .not_equal
        case:       // !
            return .exclam
        }
    case '^':       // ^
        return .carat
    case '+':       // +
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '=':   // +=
            lex_advance_cursor(ctx)
            return .add_equal
        case:       // +
            return .add
        }
    case '*':       // *
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '=':   // *=
            lex_advance_cursor(ctx)
            return .mul_equal
        case:       // *
            return .mul
        }
    case '/':       // /
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '=':   // /=
            lex_advance_cursor(ctx)
            return .div_equal
        case:       // /
            return .div
        }
    case '%':       // %
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '%':   // %%
            lex_advance_cursor(ctx)
            this_rune = lex_current_char(ctx)
            switch this_rune {
            case '=': // %%=
                lex_advance_cursor(ctx)
                return .mod_equal
            case:
                return .mod_mod
            }
        case '=':   // %=
            lex_advance_cursor(ctx)
            return .mod_equal
        case:       // %
            return .mod
        }
    case '~':       // ~
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '|':   // ~|
            lex_advance_cursor(ctx)
            this_rune = lex_current_char(ctx)
            switch this_rune {
            case '=': // ~|=
                lex_advance_cursor(ctx)
                return .nor_equal
            case:     // ~|
                return .nor
            }
        case '~':   // ~~
            lex_advance_cursor(ctx)
            return .tilde_tilde
        case '=':   // ~=
            lex_advance_cursor(ctx)
            return .xor_equal
        case:       // ~
            return .tilde
        }
    case '&':       // &
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '&':   // &&
            lex_advance_cursor(ctx)
            return .and_and
        case '=':   // &=
            lex_advance_cursor(ctx)
            return .and_equal
        case:       // %
            return .and
        }
    case '|':       // |
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '|':   // ||
            lex_advance_cursor(ctx)
            return .or_or
        case '=':   // |=
            lex_advance_cursor(ctx)
            return .or_equal
        case:       // |
            return .or
        }
    case '<':       // -
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '-':   // <-
            lex_advance_cursor(ctx)
            this_rune = lex_current_char(ctx)
            switch this_rune {
            case '>': // <->
                lex_advance_cursor(ctx)
                return .arrow_bidir
            case:
                return .arrow_left
            }
        case '<':   // <<
            lex_advance_cursor(ctx)
            this_rune = lex_current_char(ctx)
            switch this_rune {
            case '=': // <<=
                lex_advance_cursor(ctx)
                return .lshift_equal
            case:   // <<
                return .lshift
            }
        case '=':   // <=
            lex_advance_cursor(ctx)
            return .less_equal
        case:       // <
            return .less_than
        }
    case '>':       // >
        this_rune = lex_current_char(ctx)
        switch this_rune {
        case '>':   // >>
            lex_advance_cursor(ctx)
            this_rune = lex_current_char(ctx)
            switch this_rune {
            case '=': // > > =
                lex_advance_cursor(ctx)
                return .rshift_equal
            case:   // >>
                return .rshift
            }
        case '=':   // > =
            lex_advance_cursor(ctx)
            return .greater_equal
        case:       // >
            return .greater_than
        }
    case '(':       // (
        return .open_paren
    case ')':       // )
        return .close_paren
    case '{':       // {
        return .open_brace
    case '}':       // }
        return .close_brace
    case '[':       // [
        return .open_bracket
    case ']':       // ]
        return .close_bracket
    case:
    }

    return
}

get_token_substring :: proc(src: string, token: lexer_token) -> string {
    return src[token.pos.start:token.pos.offset]
}

skip_line_comment :: #force_inline proc(ctx: ^lexer) -> (r: u8, hit_eof: bool) {
    return skip_until_char(ctx, '\n')
}

skip_block_comment :: proc(ctx: ^lexer) -> (r: u8, hit_eof: bool, level: int) {
    level = 1
    for level != 0 {
        r = lex_current_char(ctx)
        if ctx.pos.offset >= len(ctx.src) {
            return r, true, level
        }
        if r == '/' && lex_peek_next_char(ctx) == '*' {
            lex_advance_cursor(ctx)
            level += 1
        }
        else if r == '*' && lex_peek_next_char(ctx) == '/' {
            lex_advance_cursor(ctx)
            level -= 1
        }
        lex_advance_cursor(ctx)
    }
    return r, false, level
}

skip_whitespace :: proc(ctx: ^lexer) -> (r: u8, hit_eof: bool) {
    for {
        r = lex_current_char(ctx)
        if ctx.pos.offset >= len(ctx.src) {
            return r, true
        }
        if !is_whitespace(r) {
            return r, false
        }
        lex_advance_cursor(ctx)
    }
}

skip_until_char :: proc(ctx: ^lexer, lookout: u8) -> (r: u8, hit_eof: bool) {
    for {
        r = lex_current_char(ctx)
        if ctx.pos.offset >= len(ctx.src) {
            return r, true
        }
        if r == lookout {
            return r, false
        }
        lex_advance_cursor(ctx)
    }
}

get_substring :: #force_inline proc(src: string, pos: position) -> string {
    return src[pos.start:pos.offset]
}

// b r u h                            b r u h
//   ^- current rune: 'r'     ->          ^- current rune: 'u'
// return current rune, move cursor forward
lex_advance_cursor :: proc(ctx: ^lexer) -> (r: u8) {
    r = ctx.src[ctx.pos.offset]
    ctx.pos.offset += 1
    ctx.pos.col+=1
    if r == '\n' {
        ctx.pos.col = 1
        ctx.pos.line += 1
    }
    return 
}

lex_peek_next_char :: proc(ctx: ^lexer) -> (r: u8) {
    return ctx.src[ctx.pos.offset+1]
}

// b r u h                             b r u h
//   ^- current rune: 'r'      ->        ^- current rune: 'r'
// return current rune, keep cursor
lex_current_char :: #force_inline proc(ctx: ^lexer) -> (r: u8) {
    // all because utf8.rune_at discards the damn byte length. fuck you utf8.rune_at. 
    //return utf8.decode_rune_in_string(ctx.src[ctx.pos.offset:])

    if ctx.pos.offset >= len(ctx.src) do return 0

    return ctx.src[ctx.pos.offset]
}

make_EOF :: #force_inline proc(ctx: ^lexer) -> lexer_token {
    return lexer_token{.EOF,ctx.pos}
}

whitespace_runes :: [?]u8{' ', '\t', '\n', '\r', '\v'}
is_whitespace :: proc(r: u8) -> bool {
    // for whitespace in whitespace_runes {
    //     if r == whitespace {
    //         return true
    //     }
    // }
    // return false

    return r == ' ' || r == '\t' || r == '\n' || r == '\r' || r == '\v'
}