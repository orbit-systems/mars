package phobos

import co "../common"
import "core:os"
import "core:time"
import "core:strings"
import "core:fmt"
import "core:path/filepath"

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//         lexer            parser                seman
// files --------> tokens ---------> parse tree ----------> AST

phobos_build_state : co.build_state

// build the complete AST of the program, spanning multiple files and modules.
construct_complete_AST :: proc() {

    // find files in compile directory and lex each of them
    if !os.exists(phobos_build_state.compile_directory) || 
       !os.is_dir(phobos_build_state.compile_directory) {
            fmt.printf("ERROR Directory \"%s\" does not exist or is not a directory.\n", phobos_build_state.compile_directory)
            os.exit(1)
    }

    compile_directory_handle, open_dir_ok := os.open(phobos_build_state.compile_directory)
    if open_dir_ok != os.ERROR_NONE {
        fmt.printf("ERROR Directory \"%s\" cannot be opened.\n", phobos_build_state.compile_directory)
        os.exit(1)
    }
    compile_directory_files, _ := os.read_dir(compile_directory_handle, 100)
    if len(compile_directory_files) < 1 {
        fmt.printf("ERROR Directory \"%s\" is empty.\n", phobos_build_state.compile_directory)
        os.exit(1)
    }

    {
        mars_files := 0
        for file in compile_directory_files {
            mars_files += int(filepath.ext(file.fullpath) == ".mars")
        }
        if mars_files < 1 {
            fmt.printf("ERROR Directory \"%s\" has no .mars files.\n", phobos_build_state.compile_directory)
            os.exit(1)
        }
    }



    lexers := make([dynamic]lexer)
    for file in compile_directory_files {
        if file.is_dir || filepath.ext(file.fullpath) != ".mars" {
            continue
        }

        file_source, _ := os.read_entire_file(file.fullpath)

        this_lexer := lexer{}

        #no_bounds_check {
            lexer_init(&this_lexer, file.name, cast(string)file_source)
            construct_token_buffer(&this_lexer)
        }
        
        append(&lexers, this_lexer)
    }



    //fmt.printf("%#v\n",compile_directory_files)

    //file_raw : []u8
    //file_path : string

    //lex: lexer
    //lexer_init(&lex, file_path, string(file_raw))
}

construct_token_buffer :: proc(ctx: ^lexer) {
    
    // ~3.5 bytes per token - this initializes the token buffer to a reasonably accurate guess of the final buffer size.
    // if the buffer needs to resize, it should only have to resize once.
    buffer_capacity_heuristic := int(f64(len(ctx.src))/3.5)
    
    ctx.buffer = make([dynamic]lexer_token, 0, buffer_capacity_heuristic)

    int_token : lexer_token
    for int_token.kind != .EOF {
        int_token = lex_next_token(ctx)
        append(&ctx.buffer, int_token)
    }

    shrink(&ctx.buffer)
}

TODO :: co.TODO