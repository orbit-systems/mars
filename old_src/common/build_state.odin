package common



build_state_t :: struct {
    compile_directory : string,
    
    
    flag_no_display_colors : bool, // do not print ANSI escape codes
    flag_runtime           : flag_runtime_setting,
}

flag_runtime_setting :: enum {
    include = 0,    // default.
    none,           // disables runtime and disallows inclusion of runtime functions
    inline,         // every instance of a runtime function is inlined
    external,       // references to runtime functions are generated but no code is included.
    replace,        // replace runtime module with another module. this module must have all runtime entities defined.
}

build_state : build_state_t