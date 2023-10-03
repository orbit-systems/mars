package phobos

import co "../common"
import "core:os"
import "core:mem"
import "core:strings"
import "core:fmt"
import "core:path/filepath"

// mars compiler frontend - lexer, parser
// produces abstract syntax tree to be passed to deimos backend

//         lexer            parser         validator
// files --------> tokens ---------> AST ------------> AST

build_state : co.build_state_t

// build the complete AST of the program, spanning multiple files and modules.
parse_program :: proc() -> ^program_tree {

    // lots of checks
    if !os.exists(build_state.compile_directory) || 
       !os.is_dir(build_state.compile_directory) {
            fmt.printf("ERROR Directory \"%s\" does not exist or is not a directory.\n", build_state.compile_directory)
            os.exit(1)
    }
    compile_directory_handle, open_dir_ok := os.open(build_state.compile_directory)
    if open_dir_ok != os.ERROR_NONE {
        fmt.printf("ERROR Directory \"%s\" cannot be opened.\n", build_state.compile_directory)
        os.exit(1)
    }
    compile_directory_files, _ := os.read_dir(compile_directory_handle, 100)
    if len(compile_directory_files) < 1 {
        fmt.printf("ERROR Directory \"%s\" is empty.\n", build_state.compile_directory)
        os.exit(1)
    }
    {
        mars_files := 0
        for file in compile_directory_files {
            mars_files += int(filepath.ext(file.fullpath) == ".mars")
        }
        if mars_files < 1 {
            fmt.printf("ERROR Directory \"%s\" has no .mars files.\n", build_state.compile_directory)
            os.exit(1)
        }
    }

    // lex all the main module files
    main_module_lexers := make([dynamic]^lexer)
    for &file in compile_directory_files {
        if file.is_dir || filepath.ext(file.fullpath) != ".mars" {
            continue
        }

        file_source, _ := os.read_entire_file(file.fullpath)
        relative_path, _ := filepath.rel(build_state.compile_directory, file.fullpath)

        this_lexer := new_lexer(relative_path, cast(string) file_source)

        #no_bounds_check {
        construct_token_buffer(this_lexer)
        }
        append(&main_module_lexers, this_lexer)
    }

    program := new_program()

    // parse all the main module's files
    main_module := new_module(program, build_state.compile_directory)
    for &l in main_module_lexers {

        f := new_file(main_module, l.path, l.src)
        p := new_parser(f, l)

        parse_file(p)
    }


    return program
}

TODO :: co.TODO