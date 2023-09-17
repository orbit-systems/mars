package phobos

import "core:slice"
import co "../common"

is_type_expr :: proc(expr : ^AST) -> bool {
    //TODO("(sandwich) is_type_expr")

    #partial switch e in expr^ {
    case 
        basic_type_expr,
        array_type_expr,
        slice_type_expr,
        pointer_type_expr,
        funcptr_type_expr,
        struct_type_expr,
        union_type_expr,
        enum_type_expr:
            return true
    }

    return false
}

type_size_and_align :: proc(type: ^AST) -> (int, int) {

    assert(is_type_expr(type), "type_size_and_align expr is not a type")

    #partial switch t in type^ {
    case basic_type_expr:
        switch t {
        case .invalid:
        case .implicit:
        case .none:
        case .untyped_bool, .untyped_float, .untyped_int, .untyped_null:
        case .mars_i8,  .mars_u8,  .mars_b8:  
            return 1, 1
        case .mars_i16, .mars_u16, .mars_b16, .mars_f16: 
            return 2, 2
        case .mars_i32, .mars_u32, .mars_b32, .mars_f32: 
            return 4, 4
        case .mars_i64, .mars_u64, .mars_b64, .mars_f64, .mars_rawptr: 
            return 8, 8
        }
    case pointer_type_expr:
        return 8, 8
    case funcptr_type_expr:
        return 8, 8
    case slice_type_expr:
        return 16, 16 // ptr + len
    case enum_type_expr:
        return type_size_and_align(t.backing_type)
    case array_type_expr:
        b_size, b_align := type_size_and_align(t.entry_type)
        return (t.length-1) * co.align_forward(b_size, b_align) + b_size, b_align
    case union_type_expr:
        // maximum size, maximum align
        max_size, max_align : int
        for field_type in t.field_types {
            size, align := type_size_and_align(field_type)
            max_size  = max(max_size, size)
            max_align = max(max_align, align)
        }
        return max_size, max_align
    case struct_type_expr:
        // accumulated size, maximum align
        running_size, running_align : int
        for field_type in t.field_types {
            size, align := type_size_and_align(field_type)
            running_size = size + co.align_forward(running_size, align)
            running_align = max(running_align, align)
        }
        return running_size, running_align
    }
    return 0, 0
}

// determine if an expression can be evaluated at compile time
is_constant_expr :: proc(expr : ^AST) -> bool {
    TODO("(sandwich) fuck i dont wanna implement is_constant_expr")
    return false
}