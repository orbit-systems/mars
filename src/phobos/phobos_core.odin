package phobos

import "core:os"
import "core:time"
import "core:strings"
import "core:fmt"
import "core:path/slashpath"
import "core:path/filepath"

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//        lexer            parser                seman
// file --------> tokens ---------> parse tree ----------> AST

construct_AST :: proc(module_directory : string) {

    // raw, read_ok := os.read_entire_file()
    // if read_ok == false {
    //     fmt.printf("Cannot read file \"%s\"\n", file_name)
    //     os.exit(-1)
    // }
    file_raw : []u8
    file_path : string

    lexer_context: lexer
    lexer_init(&lexer_context, file_path, string(file_raw))
}

construct_token_buffer :: proc(ctx: ^lexer) {
    
    buffer_capacity_heuristic := int(f64(len(ctx.src))/3.5)// 3.5 bytes per token - this initializes the token buffer to something 
    token_buffer := make([dynamic]lexer_token, 0, buffer_capacity_heuristic)

    int_token : lexer_token
    for int_token.kind != .EOF {
        int_token = lex_next_token(ctx)
        append(&token_buffer, int_token)
    }

    shrink(&token_buffer)
}