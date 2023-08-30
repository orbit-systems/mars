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

cmd_arg :: struct {
    key : string,
    val : string,
}

main :: proc() {
    
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

    file_name: string

    for argument, index in parsed_args {
        switch argument.key {
            case "-help": 
                print_help()
                os.exit(0)

            case: //default because odin is WEIRD
                if index == 0 && argument.key[0] != '-' {
                    file_name = argument.key
                    continue
                }
                print_help()
                os.exit(0)
        }
    }

    raw, read_ok := os.read_entire_file(file_name)
    if read_ok == false {
        fmt.printf("Cannot read file: \"%s\"\n", file_name)
        os.exit(-1)
    }

    ph.process_file(file_name, cast(string)raw)
}

print_help :: proc() {
    fmt.printf("you silly. you goober. DIE.\n")
}