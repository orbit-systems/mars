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
    
    // the build state contains everything that the compiler needs to know about the action(s) it is performing.
    // all build flags, settings, compile directory, etc.
    build_state = parse_command_line_args(os.args)
    
    ph.build_state = build_state // propogate build state to frontend
    dm.build_state = build_state // propogate build state to backend

    ph.parse_program()
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
        case "-no-color": build_state.flag_no_display_colors = true
        case "-runtime":  
            switch argument.val {
            case "include":  build_state.flag_runtime = .include
            case "none":     build_state.flag_runtime = .none
            case "inline":   build_state.flag_runtime = .inlined
            case "external": build_state.flag_runtime = .external
            }

        case:
            if index == 0 && argument.key[0] != '-' {
                e : bool
                build_state.compile_directory, e = filepath.abs(argument.key)
                if !e {
                    fmt.printf("ERROR Directory \"%s\" does not exist or is not a directory.\n",argument.key)
                    os.exit(1)
                }
                continue
            }
            print_help()
            os.exit(0)
        }
    }
    return
}

print_help :: proc() {
    fmt.println("uhhhhh help message!")
}

cmd_arg :: struct {
    key : string,
    val : string,
}

build_state : co.build_state