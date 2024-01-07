package phobos

import co "../common"
import "core:fmt"
import "core:os"
import "core:strings"

//@(private="file")
compiler_output_base :: proc(out_type: string, color: ANSI, path, src: string, pos: position, str: string, args: ..any) {
    
    if pos == {} {
        TODO("crash: empty pos to error/warn/info")
    }

    if !build_state.flag_no_display_colors do set_style(color)
    if !build_state.flag_no_display_colors do set_style(.Bold)
    fmt.print(out_type)
    if !build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(" [ %s @ %d:%d ] ", path, pos.line, pos.col)
    fmt.printf(str, ..args)
    fmt.print("\n")

    line_offset := get_line_offset(src, pos.line)
    line_string := src[line_offset:get_line_offset(src, pos.line+1)]
    if line_string[len(line_string)-1] == '\n' {
        line_string = src[line_offset:get_line_offset(src, pos.line+1)-1]
    }
    offset_from_line := pos.start - line_offset
    error_token_width := int(pos.offset-pos.start)

    fmt.println("     ┆ ")
    fmt.printf("% 4d │ ", pos.line)
    if !build_state.flag_no_display_colors do set_style(.Bold)
    fmt.printf("%v\n", line_string)
    if !build_state.flag_no_display_colors do set_style(.Reset)
    fmt.print("     ┆ ")
    fmt.print(strings.repeat(" ", int(offset_from_line), context.temp_allocator))
    
    if !build_state.flag_no_display_colors do set_style(.Bold)
    if !build_state.flag_no_display_colors do set_style(color)
    
    if error_token_width > 1 {
        fmt.print("╰")
        if error_token_width > 2 {
            fmt.print(strings.repeat("─", error_token_width-2, context.temp_allocator))
        }
        fmt.print("┴╴")
    } else {
        fmt.print("▲ ")
    }
    if !build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(str, ..args)
    fmt.print("\n")
}

bare_compiler_output_base :: proc(out_type: string, color: ANSI, path, src: string, pos: position, str: string, args: ..any) {
    if !build_state.flag_no_display_colors do set_style(color)
    if !build_state.flag_no_display_colors do set_style(.Bold)
    fmt.print(out_type)
    if !build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(" [ %s @ %d:%d ] ", path, pos.line, pos.col)
    fmt.printf(str, ..args)
    fmt.print("\n")

}

error :: proc(path, src: string, pos: position, str: string, args: ..any, no_print_line := false) {

    if no_print_line {
        bare_compiler_output_base("ERROR", .FG_Red, path, src, pos, str, ..args)
    } else {
        compiler_output_base("ERROR", .FG_Red, path, src, pos, str, ..args)
    }

    os.exit(1)
}

warning :: proc(path, src: string, pos: position, str: string, args: ..any, no_print_line := false) {
    
    if no_print_line {
        bare_compiler_output_base("WARNING", .FG_Yellow, path, src, pos, str, ..args)
    } else {
        compiler_output_base("WARNING", .FG_Yellow, path, src, pos, str, ..args)
    }

}

info :: proc(path, src: string, pos: position, str: string, args: ..any, no_print_line := false) {
    
    if no_print_line {
        bare_compiler_output_base("INFO", .FG_Blue, path, src, pos, str, ..args)
    } else {
        compiler_output_base("INFO", .FG_Blue, path, src, pos, str, ..args)
    }
}

get_line_offset :: proc(src: string, line_number: uint) -> uint {
    offset : uint
    lines : uint  = 1
    for offset < len(src) {
        if lines == line_number {
            return offset
        }
        if src[offset] == '\n' {
            lines += 1
        }
        offset += 1
    }
    
    return offset
}


set_style :: proc(code: ANSI) {
    fmt.printf("\x1b[%dm", code)
}

ANSI :: enum {

    Reset       = 0,

    Bold        = 1,
    Dim         = 2,
    Italic      = 3,

    FG_Black    = 30,
    FG_Red      = 31,
    FG_Green    = 32,
    FG_Yellow   = 33,
    FG_Blue     = 34,
    FG_Magenta  = 35,
    FG_Cyan     = 36,
    FG_White    = 37,
    FG_Default  = 39,

    BG_Black    = 40,
    BG_Red      = 41,
    BG_Green    = 42,
    BG_Yellow   = 43,
    BG_Blue     = 44,
    BG_Magenta  = 45,
    BG_Cyan     = 46,
    BG_White    = 47,
    BG_Default  = 49,
}