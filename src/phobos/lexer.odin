package phobos

import "core:fmt"
import "core:os"
import "core:unicode/utf8"

lex_next_token :: proc(ctx: ^lexer_info) -> (this_token: lexer_token) {
    //figure out how to parse next token
    if ctx == nil {
        fmt.printf("Nil context provided to lex_next_token.\n")
        return //return empty lexer token
    }

    if ctx.file_data == "" || ctx.file_name == "" {
        fmt.printf("oopsie poopsie no lexing for you! you didn't give me any information! stupid little shit\n")
        os.exit(-1)
    }

    lex_state :: enum {
        inside_whitespace,

    }

    for {
        break
    }
    
    return
}

// b r u h                             b r u h
//   ^- current rune: 'r'      ->          ^- current rune: 'u'
// return current rune, move cursor forward
lex_advance_rune :: proc(ctx: ^lexer_info) -> (r: rune) {
    
    return
}

// b r u h                             b r u h
//   ^- current rune: 'r'      ->        ^- current rune: 'r'
// return current rune, keep cursor
lex_lookahead_rune :: proc(ctx: ^lexer_info) -> (r: rune, byte_len: int) {
    r = utf8.rune_at(ctx.file_data,int(ctx.current_offset))
    
    return
}

is_whitespace :: proc(r: rune) -> bool {
    return {}
}