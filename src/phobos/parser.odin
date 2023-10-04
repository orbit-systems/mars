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
    return &(p.lex.buffer[p.curr_tok_index-offset])
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
        print(node)
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
        if node == nil {
            error(p.file.path, p.lex.src, current_token(p).pos, "fuck", no_print_line = current_token(p).kind == .EOF)
        }
    }

    return
}

parse_block_stmt :: proc(p: ^parser) -> (node: AST) {
    TODO("bruh")
    return
}


// try pratt parsing
parse_expr :: proc(p: ^parser, precedence := 0) -> (node: AST) {

    // get token
    tok := current_token(p)
    if tok.kind == .close_paren || tok.kind == .EOF || tok.kind == .semicolon {
        return nil
    }

    // get unary prefix expression
    left := parse_unary_expr(p)

    if left == nil {
        error(p.file.path, p.lex.src, current_token(p).pos, "tried really hard, but just could not parse \"%s\" (contact me)", get_substring(p.file.src, current_token(p).pos), no_print_line = current_token(p).kind == .EOF)
    }

    //fmt.println(current_token(p))
    for precedence < op_precedence(p, current_token(p)) {
        tok = current_token(p)

        left = parse_non_unary_expr(p, left, precedence)
    }

    return left
}

parse_non_unary_expr :: proc(p: ^parser, lhs: AST, precedence: int) -> (node: AST) {
    #partial switch current_token(p).kind {
    case .open_bracket: // array index
        //TODO("array indexing")

        n := new(array_index_expr)
        n.lhs = lhs
        n.open = current_token(p)

        advance_token(p)
        n.index = parse_expr(p)

        if current_token(p).kind != .close_bracket {
            error(p.file.path, p.lex.src, merge_pos(n.open.pos, current_token(p).pos), "expected ']' after array index expression, got \'%s\'", get_substring(p.file.src, current_token(p).pos))
        }
        n.close = current_token(p)
        advance_token(p)

        node = n
    case .open_paren:   // function call
        //TODO("function call")

        n := new(call_expr)
        n.lhs = lhs
        n.open = current_token(p)
        advance_token(p)
        for {
            if current_token(p).kind == .close_paren {
                break
            }

            arg := parse_expr(p)
            append(&n.args, arg)

            if current_token(p).kind != .comma {
                if current_token(p).kind == .close_paren {
                    break
                }
                error(p.file.path, p.lex.src, current_token(p).pos, "function args must be separated by a comma")
            }
            advance_token(p)
        }
        n.close = current_token(p)
        advance_token(p)

        node = n
    
    case .period:       // selector
        TODO("selector expressions")

    case .lshift, .rshift, .nor, .or, .tilde, .and,
         .add, .sub, .mul, .div, .mod, .mod_mod,
         .equal_equal, .not_equal,
         .less_than, .less_equal, .greater_than, .greater_equal,
         .and_and, .or_or, .tilde_tilde:

        n := new(op_binary_expr)
        n.op = current_token(p)
        n.lhs = lhs
        advance_token(p) 
        n.rhs = parse_expr(p, op_precedence(p, n.op))
        node = n

    case:
        error(p.file.path, p.lex.src, current_token(p).pos, "expected an operator, got \"%s\"", get_substring(p.file.src, current_token(p).pos))
    }
    return
}

op_precedence :: proc(p: ^parser, t: ^lexer_token) -> int {
    #partial switch t.kind {
    case .open_bracket, .period, .open_paren : return 6
    case .mul, .div, .mod, .mod_mod, 
         .and, .lshift, .rshift, .nor:  return 5
    case .add, .sub, .or, .tilde:       return 4
    case .equal_equal, .not_equal, 
         .less_than, .less_equal, 
         .greater_than, .greater_equal: return 3
    case .and_and:                      return 2
    case .or_or, .tilde_tilde:          return 1
    }
    return 0
}

parse_unary_expr :: proc(p: ^parser) -> (node: AST) {
    
    t := current_token(p)

    #partial switch t.kind {
    case .open_paren:
        
        node = new(paren_expr)
        node.(^paren_expr).open = current_token(p)
        
        advance_token(p)
        node.(^paren_expr).child = parse_expr(p)
        
        node.(^paren_expr).close = current_token(p)
        advance_token(p)

        if node.(^paren_expr).close.kind != .close_paren {
            error(p.file.path, p.lex.src, node.(^paren_expr).close.pos, "expected ')', got \"%s\"", get_substring(p.file.src, node.(^paren_expr).close.pos))
        }

    case .sub, .tilde, .exclam, .and, .dollar:
        //TODO("basic unary operations")
        
        node = new(op_unary_expr)
        node.(^op_unary_expr).op = current_token(p)
        
        advance_token(p)
    
        node.(^op_unary_expr).child = parse_unary_expr(p)

    case .keyword_cast, .keyword_bitcast:
        TODO("cast + bitcast")
    case .keyword_len:
        node = new(len_expr)
        node.(^len_expr).op = current_token(p)

        advance_token(p)
        if current_token(p).kind != .open_paren {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected '(' after 'len', got \'%s\'", get_substring(p.file.src, current_token(p).pos))
        }
        
        advance_token(p)
        node.(^len_expr).child = parse_expr(p)

        if current_token(p).kind != .close_paren {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected ')', got \'%s\'", get_substring(p.file.src, current_token(p).pos))
        }
        advance_token(p)

    case .keyword_base:
        node = new(base_expr)
        node.(^base_expr).op = current_token(p)

        advance_token(p)
        if current_token(p).kind != .open_paren {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected '(' after 'base', got \'%s\'", get_substring(p.file.src, current_token(p).pos))
        }

        advance_token(p)
        node.(^base_expr).child = parse_expr(p)

        if current_token(p).kind != .close_paren {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected ')', got \'%s\'", get_substring(p.file.src, current_token(p).pos), no_print_line = current_token(p).kind == .EOF)
        }
        advance_token(p)

    case .keyword_sizeof:
        node = new(sizeof_expr)
        node.(^sizeof_expr).op = current_token(p)

        advance_token(p)
        if current_token(p).kind != .open_paren {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected '(' after 'base', got \'%s\'", get_substring(p.file.src, current_token(p).pos), no_print_line = current_token(p).kind == .EOF)
        }

        advance_token(p)
        node.(^sizeof_expr).child = parse_expr(p)

        if current_token(p).kind != .close_paren {
            error(p.file.path, p.lex.src, current_token(p).pos, "expected ')', got \'%s\'", get_substring(p.file.src, current_token(p).pos), no_print_line = current_token(p).kind == .EOF)
        }
        advance_token(p)

    case .identifier, .identifier_discard:
        node = new(ident_expr)
        node.(^ident_expr).ident = get_substring(p.file.src, current_token(p).pos)
        node.(^ident_expr).tok = current_token(p)
        node.(^ident_expr).is_discard = current_token(p).kind == .identifier_discard
        advance_token(p)
    case .literal_int, .literal_float, .literal_bool, .literal_string, .literal_null:
        TODO("simple literals")
    case:
        error(p.file.path, p.lex.src, current_token(p).pos, "\'%s\' is not a unary operator/expression", get_substring(p.file.src, current_token(p).pos), no_print_line = current_token(p).kind == .EOF)
    case .close_paren, .semicolon:
        return nil
    }
    
    return
}

// merges a start and end position into a single position encompassing both.
// TODO make this better with min() and max() funcs
merge_pos :: #force_inline proc(start, end : position) -> position {
    return {start.start, end.offset, start.line, start.col}
}