package mars

import ph "./phobos" // frontend
import dm "./deimos" // backend
import mu "./utils"  // all mars utility functions

import "core:os"
import "core:time"
import "core:strings"
import "core:fmt"
import "core:path/slashpath"
import "core:path/filepath"

// mars compiler core - executes and manages frontend + backend function

main :: proc() {
    
    compile_directory: string
    
    {
        if len(os.args) < 2 {
            print_help()
            os.exit(0)
        }

        parsed_args: [dynamic]cmd_arg
        defer(delete(parsed_args))

        for argument in os.args[1:] {
            split_arg := strings.split(argument, ":")
            if len(split_arg) == 1 {
                append(&parsed_args, cmd_arg{argument, ""})
            } else {
                append(&parsed_args, cmd_arg{split_arg[0], split_arg[1]})
            }
        } 

        for argument, index in parsed_args {
            switch argument.key {
                case "-help": 
                    print_help()
                    os.exit(0)
                case "-no-color": 
                    global_build_flags.no_display_colors = true

                case:
                    if index == 0 && argument.key[0] != '-' {
                        compile_directory = argument.key
                        continue
                    }
                    print_help()
                    os.exit(0)
            }
        }
    }

    ph.construct_AST(compile_directory)
}

print_help :: proc() {
    fmt.printf("you silly. you goober. DIE.\n")
}

cmd_arg :: struct {
    key : string,
    val : string,
}

build_flags :: struct {
    compile_directory : string,
    no_display_colors : bool,
}

global_build_flags : build_flags