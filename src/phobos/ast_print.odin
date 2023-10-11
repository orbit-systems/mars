package phobos

import "core:fmt"

print :: proc{print_w_lvl, print_w_nolvl}

print_w_nolvl :: proc(node: AST, p: ^parser) {
    print_w_lvl(0, node, p)
}

print_w_lvl :: proc(lvl: int, node: AST, p: ^parser){
    #partial switch n in node {
    case ^scope_meta:
        printf_lvl(lvl, "scope_meta\n", p)
        printf_lvl(lvl+1, "is_global: %v\n", n.is_global, p)
        print(lvl+2, n.next, p)

    case ^expr_stmt:
        printf_lvl(lvl, "expr_stmt\n")
        for expr in n.exprs {
            print(lvl+1, expr, p)
        }

    case ^decl_stmt:
        printf_lvl(lvl, "decl_stmt\n")
        printf_lvl(lvl+1, "constant: %v\n", n.is_constant)
        printf_lvl(lvl+1, "lhs:\n")
        for expr in n.lhs {
            print(lvl+2, expr, p)
        }
        printf_lvl(lvl+1, "types:\n")
        for expr in n.types {
            print(lvl+2, expr, p)
        }
        printf_lvl(lvl+1, "rhs:\n")
        for expr in n.rhs {
            print(lvl+2, expr, p)
        }
    case ^assign_stmt:
        printf_lvl(lvl, "assign_stmt\n")
        printf_lvl(lvl+1, "lhs:\n")
        for expr in n.lhs {
            print(lvl+2, expr, p)
        }
        printf_lvl(lvl+1, "rhs:\n")
        for expr in n.rhs {
            print(lvl+2, expr, p)
        }

    case ^module_decl_stmt:
        printf_lvl(lvl, "module_decl_stmt\n")
    case ^paren_expr:
        printf_lvl(lvl, "paren_expr\n")
        print(lvl+1, n.child, p)
    case ^ident_expr:
        printf_lvl(lvl, "ident_expr '%s'\n", n.ident)
    case ^op_unary_expr:
        printf_lvl(lvl, "op_unary_expr '%v'\n", n.op.kind)
        print(lvl+1, n.child, p)
    case ^op_binary_expr:
        printf_lvl(lvl, "op_binary_expr '%v'\n", n.op.kind)
        print(lvl+1, n.lhs, p)
        print(lvl+1, n.rhs, p)
    case ^array_index_expr:
        printf_lvl(lvl, "array_index_expr\n")
        print(lvl+1, n.lhs, p)
        print(lvl+1, n.index, p)
    case ^call_expr:
        printf_lvl(lvl, "call_expr\n")
        print(lvl+1, n.lhs, p)
        for a in n.args {
            print(lvl+1, a, p)
        }
    case ^selector_expr:
        printf_lvl(lvl, "selector_expr\n")
        print(lvl+1, n.source, p)
        print(lvl+1, n.selector, p)


    case ^basic_literal_expr:
        printf_lvl(lvl, "basic_literal_expr '%v' (%v)\n", get_substring(p.file.src, n.tok.pos), n.tok.kind)


    case ^pointer_type:
        printf_lvl(lvl, "pointer_type\n")
        print(lvl+1, n.entry_type, p)
    case ^slice_type:
        printf_lvl(lvl, "slice_type\n")
        print(lvl+1, n.entry_type, p)
    case ^array_type:
        printf_lvl(lvl, "array_type\n")
        print(lvl+1, n.length, p)
        print(lvl+1, n.entry_type, p)
    case ^basic_type:
        printf_lvl(lvl, "basic_type '%v'\n", n^)


    case nil:
        printf_lvl(lvl, "NIL AST\n")
    case:
        printf_lvl(lvl, "UNHANDLED %v\n", n)
    }
}

printf_lvl :: proc(lvl: int, msg: string, args: ..any) {
    for _ in 0..<lvl {
        fmt.print("|   ")
    }
    fmt.printf(msg, ..args)
}

