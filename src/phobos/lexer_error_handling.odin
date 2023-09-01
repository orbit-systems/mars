package phobos

import "core:fmt"
import "core:os"
import "core:strings"

lexer_error_handler :: proc(ctx:^lexer, str: string, args: ..any, no_print_line := false)

lexer_default_error_handler :: proc(ctx:^lexer, str: string, args: ..any, no_print_line := false) {
    if !FLAG_NO_COLOR do set_style(.FG_Red)
    if !FLAG_NO_COLOR do set_style(.Bold)
    fmt.print("ERROR")
    if !FLAG_NO_COLOR do set_style(.Reset)
    fmt.printf(" [ %s @ %d : %d ] ", ctx.path, ctx.pos.line, ctx.pos.col)
    fmt.printf(str, ..args)
    fmt.print("\n")

    if no_print_line {
        free_all(context.temp_allocator)
        os.exit(1)
    }

    line_offset := get_line_offset(ctx.src, ctx.pos.line)
    line_string := ctx.src[line_offset:get_line_offset(ctx.src, ctx.pos.line+1)-1]
    offset_from_line := ctx.pos.start - line_offset
    error_token_width := int(ctx.pos.offset-ctx.pos.start)

    if !FLAG_NO_COLOR do set_style(.Bold)
    fmt.printf("    %v", line_string)
    fmt.print("\n")
    fmt.print(strings.repeat(" ", int(4+offset_from_line), context.temp_allocator))
    
    if !FLAG_NO_COLOR do set_style(.FG_Red)
    fmt.print("^") // FUCK you odin stringwriter you MAKE ME NOT USE UNICODE AND I HATE YOU
    if error_token_width > 1 {
        if error_token_width > 2 {
            fmt.print(strings.repeat("~", error_token_width - 2, context.temp_allocator))
        }
        fmt.print("^")
    }
    fmt.print("\n")
    if !FLAG_NO_COLOR do set_style(.Reset)


    free_all(context.temp_allocator)
    os.exit(1)
}

lexer_default_warning_handler :: proc(ctx:^lexer, str: string, args: ..any, no_print_line := false) {
    if !FLAG_NO_COLOR do set_style(.FG_Yellow)
    if !FLAG_NO_COLOR do set_style(.Bold)
    fmt.print("WARNING")
    if !FLAG_NO_COLOR do set_style(.Reset)
    fmt.printf(" [ %s @ %d : %d ] ", ctx.path, ctx.pos.line, ctx.pos.col)
    fmt.printf(str, ..args)
    fmt.print("\n")

    if no_print_line {
        free_all(context.temp_allocator)
        return
    }

    line_offset := get_line_offset(ctx.src, ctx.pos.line)
    line_string := ctx.src[line_offset:get_line_offset(ctx.src, ctx.pos.line+1)-1]
    offset_from_line := ctx.pos.start - line_offset
    error_token_width := int(ctx.pos.offset-ctx.pos.start)

    if !FLAG_NO_COLOR do set_style(.Bold)
    fmt.printf("    %v", line_string)
    fmt.print("\n")
    fmt.print(strings.repeat(" ", int(4+offset_from_line), context.temp_allocator))
    
    if !FLAG_NO_COLOR do set_style(.FG_Yellow)
    fmt.print("^")
    if error_token_width > 1 {
        if error_token_width > 2 {
            fmt.print(strings.repeat("~", error_token_width - 2, context.temp_allocator))
        }
        fmt.print("^")
    }
    fmt.print("\n")
    if !FLAG_NO_COLOR do set_style(.Reset)


    free_all(context.temp_allocator)

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