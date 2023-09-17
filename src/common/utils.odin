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

TODO :: proc(msg: string) {
    fmt.println("TODO", msg)
    os.exit(1)
}