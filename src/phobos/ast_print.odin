package phobos

import "core:fmt"

print :: proc{print_w_lvl, print_w_nolvl}

print_w_nolvl :: proc(node: AST) {
    print_w_lvl(0, node)
}

print_w_lvl :: proc(lvl: int, node: AST){
    #partial switch n in node {
    case ^scope_meta:
        printf_lvl(lvl, "scope_meta\n")
        printf_lvl(lvl, "| is_global: %v\n", n.is_global)
        print(lvl+1, n.next)
    case ^module_decl_stmt:
        printf_lvl(lvl, "module_decl_stmt\n")
    case ^paren_expr:
        printf_lvl(lvl, "paren_expr\n")
        print(lvl+1, n.child)
    case ^ident_expr:
        printf_lvl(lvl, "ident_expr \'%s\'\n", n.ident)
    case ^op_unary_expr:
        printf_lvl(lvl, "op_unary_expr \'%v\'\n", n.op.kind)
        print(lvl+1, n.child)
    case ^op_binary_expr:
        printf_lvl(lvl, "op_binary_expr \'%v\'\n", n.op.kind)
        print(lvl+1, n.lhs)
        print(lvl+1, n.rhs)
    case ^array_index_expr:
        printf_lvl(lvl, "array_index_expr\n")
        print(lvl+1, n.lhs)
        print(lvl+1, n.index)
    case ^call_expr:
        printf_lvl(lvl, "call_expr\n")
        print(lvl+1, n.lhs)
        for a in n.args {
            print(lvl+1, a)
        }
    case nil:
        printf_lvl(lvl, "NIL\n")
    case:
        printf_lvl(lvl, "UNHANDLED : %v\n", n)
    }
}

printf_lvl :: proc(lvl: int, msg: string, args: ..any) {
    for _ in 0..<lvl {
        fmt.print("|\t")
    }
    fmt.printf(msg, ..args)
}