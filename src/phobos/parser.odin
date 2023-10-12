package phobos

import "core:fmt"

parser :: struct {
    file : ^file,
    lex  : ^lexer,

    curr_tok_index : int,
    current_token : ^lexer_token,

    curr_scope : ^scope_meta,

    directive_stack : [dynamic]AST,
}

new_parser :: proc(f: ^file, l: ^lexer) -> (p: ^parser) {
    p = new(parser)
    p.file = f
    p.lex = l
    p.current_token = &(p.lex.buffer[p.curr_tok_index])

    p.directive_stack = make([dynamic]AST)

    return
}

// current_token :: proc(p: ^parser) -> ^lexer_token {
//     return &(p.lex.buffer[p.curr_tok_index])
// }

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

advance_token :: proc(p: ^parser, offset : int = 1) -> (t: ^lexer_token) {
    t = &(p.lex.buffer[p.curr_tok_index])
    p.curr_tok_index += offset
    p.current_token = &(p.lex.buffer[p.curr_tok_index])
    return t
}

advance_until :: proc(p: ^parser, kind: token_kind) -> ^lexer_token {
    t := &(p.lex.buffer[p.curr_tok_index])
    for p.lex.buffer[p.curr_tok_index].kind != kind {
        if p.lex.buffer[p.curr_tok_index].kind == .EOF {}
        advance_token(p)
    }
    return t
}

parse_file :: proc(p: ^parser) {

    append(&p.file.stmts, parse_module_decl(p))

    //TODO("fucking everything")
    for p.current_token.kind != .EOF {
        node := parse_stmt(p)
        print(node, p)
        append(&p.file.stmts, node)
    }

}

// TODO this function is old, i should update this
parse_module_decl :: proc(p: ^parser, require_semicolon := true) -> (node: AST) {

    module_declaration := new_module_decl_stmt(nil,nil)

    module_declaration.start = p.current_token

    // module name;
    // ^~~~~^
    if p.current_token.kind != .keyword_module {
        error(p.file.path, p.lex.src, p.current_token.pos, "expected module declaration, got '%v'", p.current_token.kind, no_print_line = p.current_token.kind == .EOF)
    }

    advance_token(p)
    // module name;
    //        ^~~^
    if p.current_token.kind != .identifier {
        if p.current_token.kind == .identifier_discard {
            error(p.file.path, p.lex.src, p.current_token.pos, "module name cannot be blank identifer \'_\'" )
        } else {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected module name, got \'%v\'", get_substring(p.file.src, p.current_token.pos))
        }
    }

    p.file.module.name = get_substring(p.file.src, p.current_token.pos)
    
    advance_token(p)

    if !require_semicolon {
        
        module_declaration.end = p.current_token
        add_global_stmt(p.file, module_declaration)
        
        return
    }

    // module name;
    //            ^
    if p.current_token.kind != .semicolon {
        error(p.file.path, p.lex.src, p.current_token.pos, "expected semicolon after module declaration, got %s", p.current_token.kind)
    }

    module_declaration.end = p.current_token
    //

    advance_token(p)

    return module_declaration
}

parse_external_block :: proc(p: ^parser) {
    
}

parse_stmt :: proc(p: ^parser, require_semicolon : bool = true) -> (node: AST) {

    #partial switch p.current_token.kind {
    case .keyword_module:
        if len(p.file.module.name) == 0 {
            error(p.file.path, p.lex.src, p.current_token.pos, "module already declared", no_print_line = p.current_token.kind == .EOF)
        }
    case .semicolon:
        advance_token(p)

    case .keyword_if: TODO("if stmt")
    case .keyword_for: TODO("for stmt")
    case .keyword_while: TODO("while stmt")
    case .keyword_import: TODO("import stmt")
    case .keyword_asm: TODO("asm stmt")
    case .keyword_external: TODO("external stmt")


    case: // probably a declaration or assignment or smth like that

        // left hand expr list
        lhs := make([dynamic]AST)

        for {
            expr := parse_expr(p)

            append(&lhs, expr)
            if p.current_token.kind != .comma {
                break
            }
            advance_token(p)
        }

        switch {
        case p.current_token.kind == .colon:
            n := new(decl_stmt)
            n.lhs = lhs
            advance_token(p)
            explicit_type := p.current_token.kind != .equal && p.current_token.kind != .colon
            // if explicit_type {
            //     types := make([dynamic]AST)
            //     for {
            //         type := parse_expr(p)
            //         if type != nil {
            //             error(p.file.path, p.lex.src, p.current_token.pos, "expected '=' or type expression")
            //         }

            //         append(&types, type)
            //         if p.current_token.kind != .comma {
            //             break
            //         }
            //         advance_token(p)
            //     }
            //     n.types = types
            // }
            if explicit_type {
                types := make([dynamic]AST)
                type := parse_expr(p)
                if type == nil {
                    error(p.file.path, p.lex.src, p.current_token.pos, "expected '=' or type expression")
                }
                append(&types, type)
                n.types = types
            }


            initial_vals := p.current_token.kind == .equal || p.current_token.kind == .colon
            n.is_constant = p.current_token.kind == .colon
            if initial_vals {
                rhs := make([dynamic]AST)
                advance_token(p)
                for {

                    expr := parse_expr(p)

                    if expr == nil {
                        error(p.file.path, p.lex.src, p.current_token.pos, "expected expression")
                    }

                    append(&rhs, expr)
                    if p.current_token.kind != .comma {
                        break
                    }
                    advance_token(p)
                }
                n.rhs = rhs
            }

            if require_semicolon {
                if p.current_token.kind != .semicolon {
                    error(p.file.path, p.lex.src, p.current_token.pos, "expected ';' after declaration")
                }
                advance_token(p)
            }

            node = n
        case p.current_token.kind == .equal:
            n := new(assign_stmt)
            n.lhs = lhs

            rhs := make([dynamic]AST)
            advance_token(p)
            for {
                expr := parse_expr(p)

                if expr == nil {
                    error(p.file.path, p.lex.src, p.current_token.pos, "expected expression")
                }

                append(&rhs, expr)
                if p.current_token.kind != .comma {
                    break
                }
                advance_token(p)
            }
            n.rhs = rhs

            if require_semicolon {
                if p.current_token.kind != .semicolon {
                    error(p.file.path, p.lex.src, p.current_token.pos, "expected ';' after declaration")
                }
                advance_token(p)
            }

            node = n
            // TODO("ASSIGN STMT")
        
        case p.current_token.kind > .meta_compound_assign_begin && p.current_token.kind < .meta_compound_assign_end:

            n := new(compound_assign_stmt)
            n.lhs = lhs
            n.op = p.current_token

            rhs := make([dynamic]AST)
            advance_token(p)
            for {
                expr := parse_expr(p)

                if expr == nil {
                    error(p.file.path, p.lex.src, p.current_token.pos, "expected expression")
                }

                append(&rhs, expr)
                if p.current_token.kind != .comma {
                    break
                }
                advance_token(p)
            }
            n.rhs = rhs

            if require_semicolon {
                if p.current_token.kind != .semicolon {
                    error(p.file.path, p.lex.src, p.current_token.pos, "expected ';' after declaration")
                }
                advance_token(p)
            }

            node = n

            // TODO("compound assignments")

        case p.current_token.kind == .semicolon:
            n := new(expr_stmt)
            n.exprs = lhs
            node = n
        case:
            error(p.file.path, p.lex.src, p.current_token.pos, "expected ':', '=', or ';'")
        }
    
    case .EOF:
        return nil

    }

    return
}

parse_expr :: proc(p: ^parser, precedence := 0) -> (node: AST) {

    // get token
    tok := p.current_token
    if tok.kind == .close_paren || 
       tok.kind == .close_brace || 
       tok.kind == .close_bracket || 
       tok.kind == .EOF || 
       tok.kind == .comma || 
       tok.kind == .semicolon {
        return nil
    }

    // get unary prefix expression
    diag := p.current_token
    left := parse_unary_expr(p)

    if left == nil {
        fmt.println(diag)
        error(p.file.path, p.lex.src, p.current_token.pos, "could not parse '%s' (contact me)", get_substring(p.file.src, p.current_token.pos), no_print_line = p.current_token.kind == .EOF)
    }

    //fmt.println(p.current_token)
    for precedence < op_precedence(p, p.current_token) {
        tok = p.current_token

        left = parse_non_unary_expr(p, left, precedence)
    }

    return left
}

parse_non_unary_expr :: proc(p: ^parser, lhs: AST, precedence: int) -> (node: AST) {
    #partial switch p.current_token.kind {
    case .open_bracket: // array index
        //TODO("array indexing")

        n := new(array_index_expr)
        n.lhs = lhs
        n.open = p.current_token

        advance_token(p)
        n.index = parse_expr(p)

        if p.current_token.kind != .close_bracket {
            error(p.file.path, p.lex.src, merge_pos(n.open.pos, p.current_token.pos), "expected ']' after array index expression, got '%s'", get_substring(p.file.src, p.current_token.pos))
        }
        n.close = p.current_token
        advance_token(p)

        return n

    case .open_paren:   // function call
        //TODO("function call")

        n := new(call_expr)
        n.lhs = lhs
        n.open = p.current_token
        advance_token(p)
        for {
            if p.current_token.kind == .close_paren {
                break
            }

            arg := parse_expr(p)
            append(&n.args, arg)

            if p.current_token.kind != .comma {
                if p.current_token.kind == .close_paren {
                    break
                }
                error(p.file.path, p.lex.src, p.current_token.pos, "expected ',' or ')' ")
            }
            advance_token(p)
        }
        n.close = p.current_token
        advance_token(p)

        return n
    
    case .period:       // selector
        n := new(selector_expr)
        n.op = p.current_token
        n.source = lhs
        advance_token(p) 
        n.selector = parse_expr(p, op_precedence(p, n.op))
        if n.selector == nil {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected operand after '%s'", get_substring(p.file.src, n.op.pos))
        }

        return n

    case .lshift, .rshift, .nor, .or, .tilde, .and,
         .add, .sub, .mul, .div, .mod, .mod_mod,
         .equal_equal, .not_equal,
         .less_than, .less_equal, .greater_than, .greater_equal,
         .and_and, .or_or, .tilde_tilde:

        n := new(op_binary_expr)
        n.op = p.current_token
        n.lhs = lhs
        advance_token(p) 
        n.rhs = parse_expr(p, op_precedence(p, n.op))
        if n.rhs == nil {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected operand after '%s'", get_substring(p.file.src, n.op.pos))
        }
        return n

    case:
        error(p.file.path, p.lex.src, p.current_token.pos, "expected an operator, got '%s'", get_substring(p.file.src, p.current_token.pos))
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
    
    t := p.current_token

    #partial switch t.kind {

    case .open_brace:
        n := new(compound_literal_expr)

        n.type = new(basic_type)
        n.type.(^basic_type)^ = .unresolved

        n.open = p.current_token

        advance_token(p)

        for {
            if p.current_token.kind == .close_brace {
                break
            }
            expr := parse_expr(p)
            append(&n.members, expr)

            if p.current_token.kind != .comma {
                if p.current_token.kind == .close_brace {
                    break
                }
                error(p.file.path, p.lex.src, p.current_token.pos, "expected ',' or '}' ")
            }
            advance_token(p)
        }
        n.close = p.current_token

        node = n

        advance_token(p)

    case .uninit:
        n := new(basic_literal_expr)

        n.type = new(basic_type)
        n.type.(^basic_type)^ = .unresolved

        n.tok = p.current_token

        node = n

        advance_token(p)

    case .open_paren:
        
        node = new(paren_expr)
        node.(^paren_expr).open = p.current_token
        
        advance_token(p)
        node.(^paren_expr).child = parse_expr(p)
        
        node.(^paren_expr).close = p.current_token
        advance_token(p)

        if node.(^paren_expr).close.kind != .close_paren {
            error(p.file.path, p.lex.src, node.(^paren_expr).close.pos, "expected ')', got '%s'", get_substring(p.file.src, node.(^paren_expr).close.pos))
        }

    case .sub, .tilde, .exclam, .and, .dollar:
        //TODO("basic unary operations")
        
        node = new(op_unary_expr)
        node.(^op_unary_expr).op = p.current_token
        
        advance_token(p)
    
        node.(^op_unary_expr).child = parse_unary_expr(p)

    case .keyword_cast, .keyword_bitcast:
        TODO("cast + bitcast")
    case .keyword_len:
        node = new(len_expr)
        node.(^len_expr).op = p.current_token

        advance_token(p)
        if p.current_token.kind != .open_paren {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected '(' after 'len', got '%s'", get_substring(p.file.src, p.current_token.pos))
        }
        
        advance_token(p)
        node.(^len_expr).child = parse_expr(p)

        if p.current_token.kind != .close_paren {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected ')', got '%s'", get_substring(p.file.src, p.current_token.pos))
        }
        advance_token(p)

    case .keyword_base:
        node = new(base_expr)
        node.(^base_expr).op = p.current_token

        advance_token(p)
        if p.current_token.kind != .open_paren {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected '(' after 'base', got '%s'", get_substring(p.file.src, p.current_token.pos))
        }

        advance_token(p)
        node.(^base_expr).child = parse_expr(p)

        if p.current_token.kind != .close_paren {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected ')', got '%s'", get_substring(p.file.src, p.current_token.pos), no_print_line = p.current_token.kind == .EOF)
        }
        advance_token(p)

    case .keyword_sizeof:
        node = new(sizeof_expr)
        node.(^sizeof_expr).op = p.current_token

        advance_token(p)
        if p.current_token.kind != .open_paren {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected '(' after 'base', got '%s'", get_substring(p.file.src, p.current_token.pos), no_print_line = p.current_token.kind == .EOF)
        }

        advance_token(p)
        node.(^sizeof_expr).child = parse_expr(p)

        if p.current_token.kind != .close_paren {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected ')', got '%s'", get_substring(p.file.src, p.current_token.pos), no_print_line = p.current_token.kind == .EOF)
        }
        advance_token(p)

    case .identifier, .identifier_discard:
        node = new(ident_expr)
        node.(^ident_expr).ident = get_substring(p.file.src, p.current_token.pos)
        node.(^ident_expr).tok = p.current_token
        node.(^ident_expr).is_discard = p.current_token.kind == .identifier_discard
        advance_token(p)

    case .literal_int, .literal_float, .literal_bool, .literal_string, .literal_null:
        n := new(basic_literal_expr)
        n.tok = p.current_token
        n.type = new(basic_type)
        
        #partial switch t.kind {
        case .literal_int:   n.type.(^basic_type)^ = .untyped_int
        case .literal_float: n.type.(^basic_type)^ = .untyped_float
        case .literal_bool:  n.type.(^basic_type)^ = .untyped_bool
        case .literal_string: 
            
            TODO("string literal processing")
        case .literal_null:  n.type.(^basic_type)^ = .untyped_null
        }

        //TODO("simple literals")

        advance_token(p)
        return n
    
    case .type_keyword_int, .type_keyword_uint, .type_keyword_bool, .type_keyword_float,
        .type_keyword_i8,   .type_keyword_u8,   .type_keyword_b8,   .type_keyword_f16,
        .type_keyword_i16,  .type_keyword_u16,  .type_keyword_b16,  .type_keyword_f32,
        .type_keyword_i32,  .type_keyword_u32,  .type_keyword_b32,  .type_keyword_f64,
        .type_keyword_i64,  .type_keyword_u64,  .type_keyword_b64,  .type_keyword_addr:


        advance_token(p)
        n := new(basic_type)
        
        #partial switch t.kind {
        case .type_keyword_int:     n^ = .mars_i64
        case .type_keyword_uint:    n^ = .mars_u64
        case .type_keyword_bool:    n^ = .mars_b64
        case .type_keyword_float:   n^ = .mars_f64
        case .type_keyword_i8:      n^ = .mars_i8
        case .type_keyword_u8:      n^ = .mars_u8
        case .type_keyword_b8:      n^ = .mars_b8
        case .type_keyword_f16:     n^ = .mars_f16
        case .type_keyword_i16:     n^ = .mars_i16
        case .type_keyword_u16:     n^ = .mars_u16
        case .type_keyword_b16:     n^ = .mars_b16
        case .type_keyword_f32:     n^ = .mars_f32
        case .type_keyword_i32:     n^ = .mars_i32
        case .type_keyword_u32:     n^ = .mars_u32
        case .type_keyword_b32:     n^ = .mars_b32
        case .type_keyword_f64:     n^ = .mars_f64
        case .type_keyword_i64:     n^ = .mars_i64
        case .type_keyword_u64:     n^ = .mars_u64
        case .type_keyword_b64:     n^ = .mars_b64
        case .type_keyword_addr:    n^ = .mars_addr
        }

        return n

    case .carat:
        n := new(pointer_type)
        
        advance_token(p)
        n.entry_type = parse_unary_expr(p)

        return n

    case .open_bracket:

        advance_token(p)
        if p.current_token.kind == .close_bracket {

            n := new(slice_type)

            advance_token(p)
            n.entry_type = parse_unary_expr(p)

            return n
        } else {
            n := new(array_type)
            n.length = parse_expr(p)

            if p.current_token.kind != .close_bracket {
                error(p.file.path, p.lex.src, p.current_token.pos, "expected ']', got '%s'", get_substring(p.file.src, p.current_token.pos))
            }
            
            advance_token(p)
            n.entry_type = parse_unary_expr(p)
            return n
        }
    
    case .period:
        TODO("implicit selectors")
    case .keyword_struct:
        n := new(struct_type)

        advance_token(p)
        if p.current_token.kind != .open_brace {
            error(p.file.path, p.lex.src, p.current_token.pos, "expected '{{' after 'struct'")
        }

        advance_token(p)

        for {
            if p.current_token.kind == .close_brace {
                break
            }
            
            decl := parse_stmt(p, require_semicolon = false)

            if _, ok := decl.(^decl_stmt); !ok {
                error(p.file.path, p.lex.src, p.current_token.pos, "expression must be declaration")
            }

            append(&n.field_decls, decl)

            if p.current_token.kind != .comma {
                if p.current_token.kind == .close_brace {
                    break
                }
                error(p.file.path, p.lex.src, p.current_token.pos, "expected ',' or '}' ")
            }
            advance_token(p)
        }
        
        advance_token(p)

        node = n

        // TODO("struct type expr")
    case .keyword_union:
        TODO("union type expr")
    case .keyword_enum:
        TODO("enum type expr")
    case .keyword_fn:
        TODO("fn type expr")

    case:
        error(p.file.path, p.lex.src, p.current_token.pos, "expected operand, got '%s'", get_substring(p.file.src, p.current_token.pos), no_print_line = p.current_token.kind == .EOF)
    }
    
    return
}


// merges a start and end position into a single position encompassing both.
// TODO make this better with min() and max() funcs so that order does not matter
merge_pos :: #force_inline proc(start, end : position) -> position {
    return {
        min(start.start,  end.start),
        max(start.offset, end.offset),
        min(start.line,   end.line),
        min(start.col,    end.col),
    }
}