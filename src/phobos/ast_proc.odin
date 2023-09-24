package phobos

import "core:slice"
import "core:fmt"
import "core:strings"
import "core:runtime"
import co "../common"

is_type_expr :: proc(expr : AST) -> bool {
    //TODO("(sandwich) is_type_expr")

    #partial switch e in expr {
    case 
        ^basic_type_expr,
        ^array_type_expr,
        ^slice_type_expr,
        ^pointer_type_expr,
        ^funcptr_type_expr,
        ^struct_type_expr,
        ^union_type_expr,
        ^enum_type_expr:
            return true
    }

    return false
}

type_size_and_align :: proc(type: AST) -> (int, int) {

    assert(is_type_expr(type), "type_size_and_align expr is not a type")

    #partial switch t in type {
    case ^basic_type_expr:
        switch t^ {
        case .invalid:
        case .unresolved:
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
    case ^pointer_type_expr:
        return 8, 8
    case ^funcptr_type_expr:
        return 8, 8
    case ^slice_type_expr:
        return 16, 8 // ptr + len
    case ^enum_type_expr:
        return type_size_and_align(t.backing_type)
    case ^array_type_expr:
        b_size, b_align := type_size_and_align(t.entry_type)
        return (t.length-1) * co.align_forward(b_size, b_align) + b_size, b_align
    case ^union_type_expr:
        // maximum size, maximum align
        max_size, max_align : int
        for field_type in t.field_types {
            size, align := type_size_and_align(field_type)
            max_size  = max(max_size, size)
            max_align = max(max_align, align)
        }
        return max_size, max_align
    case ^struct_type_expr:
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
is_constant_expr :: proc(expr : AST) -> bool {
    TODO("(sandwich) fuck i dont wanna implement this (is_constant_expr)")
    return false
}

// bad + horrible but whatever - i guess its okay cause i pass in an allocator
type_to_string :: proc(expr: AST, alloc: runtime.Allocator = context.allocator) -> string {
    assert(is_type_expr(expr), "type_size_and_align expr is not a type")

    #partial switch t in expr {
    case ^basic_type_expr:
        switch t^ {
        case .invalid:       return "(invalid)"
        case .unresolved:    return "(unresolved)"
        case .none:          return "(none)"
        case .untyped_bool:  return "untyped_bool"
        case .untyped_float: return "untyped_float"
        case .untyped_int:   return "untyped_int"
        case .untyped_null:  return "untyped_null"
        case .mars_i8:       return "i8"
        case .mars_u8:       return "u8"
        case .mars_b8:       return "b8"
        case .mars_i16:      return "i16"
        case .mars_u16:      return "u16"
        case .mars_b16:      return "b16"
        case .mars_f16:      return "f16"
        case .mars_i32:      return "i32"
        case .mars_u32:      return "u32"
        case .mars_b32:      return "b32"
        case .mars_f32:      return "f32"
        case .mars_i64:      return "i64"
        case .mars_u64:      return "u64"
        case .mars_b64:      return "b64"
        case .mars_f64:      return "f64"
        case .mars_rawptr:   return "rawptr"
        }
    case ^pointer_type_expr:
        return strings.concatenate({"^",type_to_string(t.entry_type, alloc)}, alloc)
    case ^funcptr_type_expr:
        
    case ^slice_type_expr:
        return strings.concatenate({"[]",type_to_string(t.entry_type, alloc)}, alloc)
    case ^enum_type_expr:
    case ^array_type_expr:

        default := context.allocator
        context.allocator = alloc
        str := strings.concatenate({fmt.aprintf("[%d]", t.length), type_to_string(t.entry_type, alloc)}, alloc)
        context.allocator = default
        
        return str

    case ^union_type_expr:
        
    case ^struct_type_expr:

    }
    return ""
}