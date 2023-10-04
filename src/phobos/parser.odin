package phobos

import "core:fmt"

parser :: struct {
    file : ^file,
    lex  : ^lexer,

    curr_tok_index : int,

    curr_scope : ^scope_meta,

    node_stack      : [dynamic]AST,
    directive_stack : [dynamic]AST,
}

new_parser :: proc(f: ^file, l: ^lexer) -> (p: ^parser) {
    p = new(parser)
    p.file = f
    p.lex = l

    p.directive_stack = make([dynamic]AST)

    return
}

current_token :: proc(p: ^parser) -> ^lexer_token {
    return &(p.lex.buffer[p.curr_tok_index])
}

peek_token :: proc(p: ^parser, offset : int = 1) -> ^lexer_token {
    return &(p.lex.buffer[p.curr_tok_index + offset])
}

peek_until :: proc(p: ^parser, kind: token_kind) -> ^lexer_token {
    offset := 0
    for (p.lex.buffer[p.curr_tok_index + offset]).kind != kind {
        offset += 1
    }
    return &(p.lex.buffer[p.curr_tok_index + offset])
}

advance_token :: proc(p: ^parser, offset : int = 1) -> ^lexer_token {
    p.curr_tok_index += offset
    return &(p.lex.buffer[p.curr_tok_index])
}

advance_until :: proc(p: ^parser, kind: token_kind) -> ^lexer_token {
    for p.lex.buffer[p.curr_tok_index].kind != kind {
        advance_token(p)
    }
    return &(p.lex.buffer[p.curr_tok_index])
}

parse_file :: proc(p: ^parser) {

    append(&p.file.stmts, parse_module_decl(p))

    //TODO("fucking everything")
    for {
        node := parse_stmt(p)
        break
    }

}

parse_module_decl :: proc(p: ^parser, consume_semicolon := true) -> (node: AST) {

    module_declaration := new_module_decl_stmt(nil,nil)

    module_declaration.start = current_token(p)

    // module name;
    // ^~~~~^
    if current_token(p).kind != .keyword_module {
        error(p.file.path, p.lex.src, current_token(p).pos, "expected module declaration", no_print_line = current_token(p).kind == .EOF)
    }

    advance_token(p)
    // module name;
    //        ^~~^
    if current_token(p).kind != .identifier {
        if current_token(p).kind == .identifier_discard {
            error(p.file.path, p.lex.src, current_token(p).pos, "module name cannot be blank identifer \"_\"" )
        } else {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected module name (identifer)" )
        }
    }

    p.file.module.name = get_substring(p.file.src, current_token(p).pos)
    
    advance_token(p)

    if !consume_semicolon {
        
        module_declaration.end = current_token(p)
        add_global_stmt(p.file, module_declaration)
        
        return
    }

    // module name;
    //            ^
    if current_token(p).kind != .semicolon {
        error(p.file.path, p.lex.src, current_token(p).pos, "expected semicolon after module declaration, got %s", current_token(p).kind)
    }

    module_declaration.end = current_token(p)
    //

    advance_token(p)

    return module_declaration
}

parse_external_block :: proc(p: ^parser) {
    
}

parse_stmt :: proc(p: ^parser) -> (node: AST) {

    //TODO("fucking everything")

    #partial switch current_token(p).kind {
    case .keyword_module:
        if len(p.node_stack) == 0 {
            error(p.file.path, p.lex.src, current_token(p).pos, "module already declared", no_print_line = current_token(p).kind == .EOF)
        }
        
    case .semicolon:
        advance_token(p)
    case:
        node = parse_expr(p)
    }

    return
}

parse_block_stmt :: proc(p: ^parser) -> (node: AST) {
    TODO("bruh")
    return
}


parse_expr :: proc(p: ^parser) -> (node: AST) {
    return //parse_unary_expr(p)
}

// ! BIOHAZARD - OLD UNARY PARSING

// parse_unary_expr :: proc(p: ^parser) -> (node: AST) {
//     #partial switch current_token(p).kind {
//     case .close_paren:
//         return nil
//     case .open_paren:
//         node = new(paren_expr)
//         node.(^paren_expr).open = current_token(p)
//         advance_token(p)
//         if current_token(p).kind == .close_paren {
//              error(p.file.path, p.lex.src, merge_pos(node.(^paren_expr).open.pos, current_token(p).pos), "expected expression inside ()")
//         }
//         node.(^paren_expr).child = parse_expr(p)
//         if node == nil {
//             error(p.file.path, p.lex.src, current_token(p).pos, "nil expression after (")
//         }
//         if current_token(p).kind != .close_paren {
//             error(p.file.path, p.lex.src, current_token(p).pos, "expected ')'")
//         }
//         node.(^paren_expr).close = current_token(p)
//         advance_token(p)
//         return
    
//     case .keyword_len:
//         node = new(len_expr)
//         node.(^len_expr).op = current_token(p)

//         advance_token(p)
//         if current_token(p).kind != .open_paren {
//             error(p.file.path, p.lex.src, current_token(p).pos, "expected '(' after len expression")
//         }
//         advance_token(p)
//         node.(^len_expr).child = parse_expr(p)
//         if node.(^len_expr).child == nil {
//             error(p.file.path, p.lex.src, merge_pos(node.(^len_expr).op.pos, current_token(p).pos), "expected expression inside \"len()\"")
//         }
//         if current_token(p).kind != .close_paren {
//             error(p.file.path, p.lex.src, current_token(p).pos, "expected ')' after len expression")
//         }
//         advance_token(p)
//         return

//     case .keyword_base:
//         node = new(base_expr)
//         node.(^base_expr).op = current_token(p)

//         advance_token(p)
//         if current_token(p).kind != .open_paren {
//             error(p.file.path, p.lex.src, current_token(p).pos, "expected '(' after \"base\" expression")
//         }
//         advance_token(p)
//         node.(^base_expr).child = parse_expr(p)
//         if node.(^base_expr).child == nil {
//             error(p.file.path, p.lex.src, merge_pos(node.(^base_expr).op.pos, current_token(p).pos), "expected expression inside \"base()\"")
//         }
//         if current_token(p).kind != .close_paren {
//             error(p.file.path, p.lex.src, current_token(p).pos, "expected ')' after base expression")
//         }
//         advance_token(p)
//         return

//     case .keyword_cast:
//         node = new(cast_expr)
//         node.(^cast_expr).op = current_token(p)




//     case .keyword_bitcast:
//         node = new(bitcast_expr)
//         node.(^bitcast_expr).op = current_token(p)

//     case .and, .dollar, .sub, .tilde, .exclam:
//         node = new(op_unary_expr)
//         node.(^op_unary_expr).op = current_token(p)
//         advance_token(p)
//         node.(^op_unary_expr).child = parse_unary_expr(p)
//         if node.(^op_unary_expr).child == nil {
//             error(p.file.path, p.lex.src, node.(^op_unary_expr).op.pos, "expected expression after unary operation")
//         }
//         //advance_token(p)
//         return
    

//     case .identifier:
//         node = new(ident_expr)
//         node.(^ident_expr).ident = get_substring(p.file.src, current_token(p).pos)
//         node.(^ident_expr).tok = current_token(p)
//         advance_token(p)
//         return

//     case:
//         error(p.file.path, p.lex.src, current_token(p).pos, "\"%s\" is not a unary operator", get_substring(p.lex.src, current_token(p).pos))
//         //TODO("DOOPSIE POOPSIE SANDWICH MADE AN OOPSIE (contact me)")
//     }

//     return
// }


// merges a start and end position into a single position encompassing both.
merge_pos :: #force_inline proc(start, end : position) -> position {
    return {start.start, end.offset, start.line, start.col}
}