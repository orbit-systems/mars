package phobos

import "core:fmt"
import "core:os"

lex_next_token :: proc(ctx: ^lexer_info) -> lexer_token {
    //figure out how to parse next token
    if ctx == nil {
        fmt.printf("Nil context provided to lex_next_token.\n")

        return lexer_token{} //return empty lexer token
    }

    if ctx.file_data == "" {
        fmt.printf("hey what the fuck did you do! the file data is non existent! go DIE\n")
        os.exit(-1)
    }

    //determine family of token

    

}