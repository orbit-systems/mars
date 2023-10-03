package common

import "core:os"
import "core:fmt"

// useful functions throughout the compiler

// align a value forward to next alignment increment -
// taken STRAIGHT from the odin compiler lmfao
align_forward :: proc(value, align: int) -> int {
    assert(is_power_of_two(align), "alignment is not power of two FUCK")

    return ((value + (align-1)) &~ (align-1))
}
// check value is a power of two
is_power_of_two :: proc(value: int) -> bool {
    if (value <= 0) {
		return false
    }
	return !transmute(b64)(value & (value-1));
}

TODO :: proc(msg: string, args : ..any, loc := #caller_location) {

    if !build_state.flag_no_display_colors do set_style(.FG_Cyan)
    if !build_state.flag_no_display_colors do set_style(.Bold)
    fmt.print("TODO ")
    if !build_state.flag_no_display_colors do set_style(.Reset)
    fmt.printf("(%v) ", loc.procedure)
    fmt.printf(msg, ..args)
    fmt.print("\n")
    os.exit(1)
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