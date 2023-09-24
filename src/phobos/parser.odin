package phobos

// parse_file :: proc(ctx: ^lexer, module: ^module_AST) -> (node: ^file_AST) {

//     node = new(file_AST)

//     node.path = ctx.pos.path
//     node.root = new(AST)
//     node.module = module
//     ctx.curr_token = 0

//     process_module_decl(ctx, node)

//     return
// }

// process_module_decl :: proc(ctx: ^lexer, node: ^file_AST) {
//     if ctx.buffer[ctx.curr_token].kind != .keyword_module {
//         error(ctx.buffer[ctx.curr_token].pos, "expected module declaration, got %s", ctx.buffer[ctx.curr_token].kind)
//     }
//     ctx.curr_token += 1

//     if ctx.buffer[ctx.curr_token].kind != .identifier {
//         error(ctx.buffer[ctx.curr_token].pos, "module name must be valid identifier")
//     }
//     module_name := get_substring(ctx.buffer[ctx.curr_token].pos)

//     ctx.curr_token += 1

//     expect_token(ctx, .semicolon)

//     module_decl := new(AST)

//     module_decl = new_entity(module_name)
// }

expect_token :: proc(ctx: ^lexer, kind: token_kind) {
    if ctx.buffer[ctx.curr_token].kind != .semicolon {
        error(ctx.buffer[ctx.curr_token].pos, "expected %s, got %s", kind, ctx.buffer[ctx.curr_token].kind)
    }
}