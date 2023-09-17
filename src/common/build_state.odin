package common



build_state :: struct {
    compile_directory : string,
    
    
    flag_no_display_colors : bool, // do not print ANSI escape codes
    flag_inline_runtime    : bool, // force inline every instance of a runtime function
}