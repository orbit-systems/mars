package playground

// for smart_pack idea testing

entity :: struct {
    size  : int,
    align : int,
}

smart_pack :: proc (structure : []entity) -> []entity {
    
    init_size, init_align := struct_size_and_align(structure)
    packed_size : int
    for field in structure {
        packed_size += field.size
    }
    // struct is already optimally packed
    if init_size == packed_size {
        return structure
    }
    
    
    return {}
}

struct_size_and_align :: proc(type: []entity) -> (int, int) {


    // accumulated size, maximum align
    running_size, running_align : int

    for field in type {
        running_size = field.size + align_forward(running_size, field.align)
        running_align = max(running_align, field.align)
    }

    return running_size, running_align

}

align_forward :: proc(value, align: int) -> int {
    assert(is_power_of_two(align), "alignment is not power of two FUCK")

    return ((value + (align-1)) &~ (align-1))
}
// check value is a power of two
is_power_of_two :: proc(value: int) -> bool {
    if (value <= 0) {
		return false
    }
	return !cast(bool)(value & (value-1));
}