package phobos

import "core:slice"
import "core:fmt"
import "core:strings"
import "core:runtime"
import co "../common"

is_type_expr :: proc(expr : AST) -> bool {
    TODO("finish")

    #partial switch e in expr {
    case 
        ^basic_type,
        ^array_type,
        ^slice_type,
        ^pointer_type,
        ^funcptr_type,
        ^struct_type,
        ^union_type,
        ^enum_type:
            return true
    }

    return false
}

type_size_and_align :: proc(type: AST) -> (int, int) {

    assert(is_type_expr(type), "type_size_and_align expr is not a type")

    #partial switch t in type {
    case ^basic_type:
        #partial switch t^ {
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
        case .mars_i64, .mars_u64, .mars_b64, .mars_f64, .mars_addr: 
            return 8, 8
        case:
            return 0, 0
        }
    case ^pointer_type:
        return 8, 8
    case ^funcptr_type:
        return 8, 8
    case ^slice_type:
        return 16, 8 // ptr + len
    case ^enum_type:
        return type_size_and_align(t.backing_type)
    case ^array_type:
        TODO("array type size and align")
        // b_size, b_align := type_size_and_align(t.entry_type)
        // return (t.length-1) * co.align_forward(b_size, b_align) + b_size, b_align
    case ^union_type:
        // maximum size, maximum align
        max_size, max_align : int
        for field_type in t.field_types {
            size, align := type_size_and_align(field_type)
            max_size  = max(max_size, size)
            max_align = max(max_align, align)
        }
        return max_size, max_align
    case ^struct_type:
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
    TODO("(sandwich) fuck i dont wanna implement this yet (is_constant_expr)")
    return false
}

// bad + horrible but whatever - i guess its okay cause i pass in an allocator
type_to_string :: proc(expr: AST, alloc: runtime.Allocator = context.allocator) -> string {
    assert(is_type_expr(expr), "type_size_and_align expr is not a type")

    #partial switch t in expr {
    case ^basic_type:
        switch t^ {
        case .invalid:          return "(invalid)"
        case .unresolved:       return "(unresolved)"
        case .none:             return "(none)"
        case .untyped_bool:     return "untyped_bool"
        case .untyped_float:    return "untyped_float"
        case .untyped_int:      return "untyped_int"
        case .untyped_null:     return "untyped_null"
        case .mars_i8:          return "i8"
        case .mars_u8:          return "u8"
        case .mars_b8:          return "b8"
        case .mars_i16:         return "i16"
        case .mars_u16:         return "u16"
        case .mars_b16:         return "b16"
        case .mars_f16:         return "f16"
        case .mars_i32:         return "i32"
        case .mars_u32:         return "u32"
        case .mars_b32:         return "b32"
        case .mars_f32:         return "f32"
        case .mars_i64:         return "i64"
        case .mars_u64:         return "u64"
        case .mars_b64:         return "b64"
        case .mars_f64:         return "f64"
        case .mars_addr:        return "addr"
        case .internal_lib:     return "internal_lib"
        case .internal_type:    return "internal_type"
        }
    case ^pointer_type:
        return strings.concatenate({"^",type_to_string(t.entry_type, alloc)}, alloc)
    case ^funcptr_type:
        
    case ^slice_type:
        return strings.concatenate({"[]",type_to_string(t.entry_type, alloc)}, alloc)
    case ^enum_type:
        TODO("type_to_string ENUM")
    case ^array_type:
        default := context.allocator
        context.allocator = alloc
        str := strings.concatenate({fmt.aprintf("[%d]", t.length), type_to_string(t.entry_type, alloc)}, alloc)
        context.allocator = default
        
        return str

    case ^union_type:
        TODO("type_to_string UNION")
    case ^struct_type:
        TODO("type_to_string STRUCT")
    }
    return "BAD TYPE"
}

can_cast :: proc(from, to : AST) -> bool {
    TODO("why is it called oven when you of in the cold food of out hot eat the food")
    return false
}

can_bitcast :: proc(from, to : AST) -> bool {
    TODO("why is it called oven when you of in the cold food of out hot eat the food")
    return false
}


// destroy() recursively deletes AST nodes. make sure it isn't cyclic!
destroy :: proc{destroy_AST, destroy_dynarr_AST}

destroy_AST :: proc(node: AST) {
    TODO("why is it called oven when you of in the cold food of out hot eat the food")
}

destroy_dynarr_AST :: proc(nodes: [dynamic]AST) {
    for n in nodes {
        destroy(n)
    }
    delete(nodes)

    TODO("why is it called oven when you of in the cold food of out hot eat the food")
}

// make_pos :: proc(node: AST) -> position {
//     switch n in node {
//     case ^decl_stmt:
//         return merge_pos(n.lhs[0])
//     case:
//         return {}
//     }
// }