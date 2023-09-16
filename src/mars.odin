package mars

import ph "phobos" // frontend
import dm "deimos" // backend
import co "common" // all mars utility functions

import "core:os"
import "core:time"
import "core:strings"
import "core:fmt"
import "core:path/filepath"

// mars compiler core - executes and manages frontend + backend function

main :: proc() {
    
    // the build state contains everything that the compiler needs to know about the action it is performing.
    // all build flags, settings, compile directory, etc.
    global_build_state = parse_command_line_args(os.args)
    
    ph.phobos_build_state = global_build_state // propogate build state to front-end
    dm.deimos_build_state = global_build_state // propogate build state to back-end
    ph.is_constant_expr(nil)
    ph.construct_complete_AST()
}

parse_command_line_args :: proc(args: []string) -> (build_state: co.build_state) {
    if len(args) < 2 {
        print_help()
        os.exit(0)
    }
    parsed_args: [dynamic]cmd_arg
    defer(delete(parsed_args))
    for argument in args[1:] {
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
        case "-no-color":       build_state.flag_no_display_colors = true
        case "-inline-runtime": build_state.flag_inline_runtime = true

        case:
            if index == 0 && argument.key[0] != '-' {
                build_state.compile_directory = argument.key
                continue
            }
            print_help()
            os.exit(0)
        }
    }
    return
}

print_help :: proc() {
    fmt.printf("you silly. you goober. DIE.\n")
}

cmd_arg :: struct {
    key : string,
    val : string,
}

global_build_state : co.build_state