package phobos

import co "../common"
import "core:fmt"
import "core:os"
import "core:strings"

error :: proc(path, src: string, pos: position, str: string, args: ..any, no_print_line := false) {
    if !phobos_build_state.flag_no_display_colors do set_style(.FG_Red)
    if !phobos_build_state.flag_no_display_colors do set_style(.Bold)
    fmt.print("ERROR")
    if !phobos_build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(" [ %s @ %d:%d ] ", path, pos.line, pos.col)
    fmt.printf(str, ..args)
    fmt.print("\n")


    if no_print_line {
        os.exit(1)
    }

    line_offset := get_line_offset(src, pos.line)
    line_string := src[line_offset:get_line_offset(src, pos.line+1)-1]
    offset_from_line := pos.start - line_offset
    error_token_width := int(pos.offset-pos.start)

    fmt.println("     ┆ ")
    fmt.printf("% 4d │ ", pos.line)
    if !phobos_build_state.flag_no_display_colors do set_style(.Bold)
    fmt.printf("%v\n", line_string)
    if !phobos_build_state.flag_no_display_colors do set_style(.Reset)
    fmt.print("     ┆ ")
    fmt.print(strings.repeat(" ", int(offset_from_line), context.temp_allocator))
    
    if !phobos_build_state.flag_no_display_colors do set_style(.Bold)
    if !phobos_build_state.flag_no_display_colors do set_style(.FG_Red)
    
    if error_token_width > 1 {
        fmt.print("╰")
        if error_token_width > 2 {
            fmt.print(strings.repeat("─", error_token_width-2, context.temp_allocator))
        }
        fmt.print("┴╴")
    } else {
        fmt.print("↑ ")
    }
    if !phobos_build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(str, ..args)
    fmt.print("\n")

    os.exit(1)
}

warning :: proc(path, src: string, pos: position, str: string, args: ..any, no_print_line := false) {
    if !phobos_build_state.flag_no_display_colors do set_style(.FG_Yellow)
    if !phobos_build_state.flag_no_display_colors do set_style(.Bold)
    fmt.print("WARNING")
    if !phobos_build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(" [ %s @ %d:%d ] ", path, pos.line, pos.col)
    fmt.printf(str, ..args)
    fmt.print("\n")

    defer free_all(context.temp_allocator)

    if no_print_line {
        return
    }

    line_offset := get_line_offset(src, pos.line)
    line_string := src[line_offset:get_line_offset(src, pos.line+1)-1]
    offset_from_line := pos.start - line_offset
    error_token_width := int(pos.offset-pos.start)

    fmt.println("     │ ")
    fmt.printf("% 4d │ ", pos.line)
    if !phobos_build_state.flag_no_display_colors do set_style(.Bold)
    fmt.printf("%v\n", line_string)
    if !phobos_build_state.flag_no_display_colors do set_style(.Reset)
    fmt.print("     │ ")
    fmt.print(strings.repeat(" ", int(offset_from_line), context.temp_allocator))
    
    if !phobos_build_state.flag_no_display_colors do set_style(.Bold)
    if !phobos_build_state.flag_no_display_colors do set_style(.FG_Yellow)
    
    if error_token_width > 1 {
        fmt.print("╰")
        if error_token_width > 2 {
            fmt.print(strings.repeat("─", error_token_width-2, context.temp_allocator))
        }
        fmt.print("┴╴")
    } else {
        fmt.print("↑ ")
    }
    if !phobos_build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf(str, ..args)
    fmt.print("\n")

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