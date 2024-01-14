#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

#define current_token ((p)->tokens.raw[(p)->current_tok])
#define peek_token(n) ((p)->tokens.raw[(p)->current_tok + (n)])
#define advance_token (((p)->current_tok + 1 < (p)->tokens.len) ? ((p)->current_tok)++ : 0)
#define advance_n_tok(n) (((p)->current_tok + n < (p)->tokens.len) ? ((p)->current_tok)+=n : 0)

#define error_at_parser(p, message, ...) \
    error_at_string((p)->path, (p)->src, current_token.text, message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token_index(p, index, message, ...) \
    error_at_string((p)->path, (p)->src, (p)->tokens.raw[index].text, message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token(p, token, message, ...) \
    error_at_string((p)->path, (p)->src, (token).text, message __VA_OPT__(,) __VA_ARGS__)

// construct a parser struct from a lexer and an arena allocator
parser make_parser(lexer* restrict l, arena alloca) {
    parser p = {0};
    p.alloca = alloca;
    p.tokens = l->buffer;
    p.path   = l->path;
    p.src    = l->src;
    p.current_tok = 0;

    return p;
}

AST parse_module_decl(parser* restrict p) {

    AST n = new_ast_node(&p->alloca, astype_module_decl);

    if (current_token.type != tt_keyword_module)
        error_at_parser(p, "expected module declaration, got %s", token_type_str[current_token.type]);

    n.as_module_decl->base.start = &current_token;

    advance_token;
    if (current_token.type != tt_identifier)
        error_at_parser(p, "expected module name, got %s", token_type_str[current_token.type]);

    n.as_module_decl->name = &current_token;

    advance_token;
    if (current_token.type != tt_semicolon)
        error_at_parser(p, "expected ';', got %s", token_type_str[current_token.type]);
    
    n.as_module_decl->base.end = &current_token;
    advance_token;
    return n;
}

void parse_file(parser* restrict p) {
    
    p->module_decl = parse_module_decl(p);

    p->head = new_ast_node(&p->alloca, astype_block_stmt);
    AST smth = parse_stmt(p);
}

AST parse_expr(parser* restrict p) {
    AST n = NULL_AST;
    n = parse_binary_expr(p, 0);
    return n;
}

AST parse_binary_expr(parser* restrict p, int precedence) {
    AST n = NULL_AST;
    n = parse_unary_expr(p);
    return n;
}

AST parse_unary_expr(parser* restrict p) {
    AST n = NULL_AST;
    
    switch (current_token.type) {
    case tt_sub:
    case tt_tilde:
    case tt_exclam:
    case tt_and:
    case tt_keyword_sizeof:
    case tt_keyword_alignof:
    case tt_keyword_offsetof:
    case tt_keyword_inline:
        n = new_ast_node(&p->alloca, astype_unary_op_expr);
        n.as_unary_op_expr->base.start = &current_token;
        n.as_unary_op_expr->op = &current_token;
        advance_token;

        n.as_unary_op_expr->inside = parse_unary_expr(p);

        n.as_unary_op_expr->base.end = &peek_token(-1);
        break;
    
    case tt_keyword_cast:
    case tt_keyword_bitcast:
        n = new_ast_node(&p->alloca, astype_cast_expr);
        n.as_cast_expr->base.start = &current_token;
        n.as_cast_expr->is_bitcast = current_token.type == tt_keyword_bitcast;
        advance_token;

        if (current_token.type != tt_open_paren)
            error_at_parser(p, "expected '(' after 'cast', got %s", token_type_str[current_token.type]);
        advance_token;

        n.as_cast_expr->type = parse_expr(p);

        if (current_token.type != tt_open_paren)
            error_at_parser(p, "expected ')', got %s", token_type_str[current_token.type]);
        advance_token;

        n.as_cast_expr->rhs = parse_unary_expr(p);

        n.as_cast_expr->base.end = &peek_token(-1);
        break;
    default:
        n = parse_atomic_expr(p);
    }
    return n;
}

i64 ascii_to_digit_val(parser* restrict p, char c, u8 base) {
    if (c >= '0' && c <= '9') c = c-'0';
    if (c >= 'a' && c <= 'f') c = c-'a' +10;
    if (c >= 'A' && c <= 'F') c = c-'A' +10;
    
    if (c >= base)
        error_at_parser(p, "invalid base %d digit '%c'", base, c);
    return c;
}

i64 char_lit_value(parser* restrict p) {
    string t = current_token.text;

    if (t.raw[1] != '\\') { // trivial case
        if (t.len > 3) error_at_parser(p, "char literal too long");
        return t.raw[2];
    }

    switch(t.raw[2]) {
    case '0':  return '\0';
    case 'a':  return '\a';
    case 'b':  return '\b';
    case 'e':  return '\e';
    case 'f':  return '\f';
    case 'n':  return '\n';
    case 'r':  return '\r';
    case 't':  return '\t';
    case 'v':  return '\v';
    case '\\': return '\\';
    case '\"': return '\"';
    case '\'': return '\'';
    case 'x':
        if (t.len > 6) error_at_parser(p, "char literal too long");
        return ascii_to_digit_val(p, t.raw[3], 16) * 0x10 + ascii_to_digit_val(p, t.raw[4], 16);
    case 'u':
        if (t.len > 8) error_at_parser(p, "char literal too long");
        return ascii_to_digit_val(p, t.raw[3], 16) * 0x1000 + ascii_to_digit_val(p, t.raw[4], 16) * 0x0100 + 
               ascii_to_digit_val(p, t.raw[5], 16) * 0x0010 + ascii_to_digit_val(p, t.raw[6], 16);
    case 'U':
        if (t.len > 12) error_at_parser(p, "char literal too long");
        return ascii_to_digit_val(p, t.raw[3], 16) * 0x10000000 + ascii_to_digit_val(p, t.raw[4], 16) * 0x01000000 + 
               ascii_to_digit_val(p, t.raw[5], 16) * 0x00100000 + ascii_to_digit_val(p, t.raw[6], 16) * 0x00010000 +
               ascii_to_digit_val(p, t.raw[7], 16) * 0x00001000 + ascii_to_digit_val(p, t.raw[8], 16) * 0x00000100 + 
               ascii_to_digit_val(p, t.raw[9], 16) * 0x00000010 + ascii_to_digit_val(p, t.raw[10], 16);

    default:
        error_at_parser(p, "invalid escape sequence '%s'", clone_to_cstring(substring_len(t, 1, t.len-2)));
    }
    return -1;
}

// a recursive method is probably the best way to do this tbh
AST parse_atomic_expr(parser* restrict p) {
    AST n = NULL_AST;

    bool out = false;
    while (!out) {
        switch (current_token.type) {
        case tt_literal_null:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;

            n.as_literal_expr->value.kind = ev_bool; 
            n.as_literal_expr->value.as_bool = string_eq(current_token.text, to_string("true"));

            advance_token;
            break;
        case tt_literal_bool:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;

            n.as_literal_expr->value.kind = ev_bool;
            n.as_literal_expr->value.as_bool = string_eq(current_token.text, to_string("true"));

            advance_token;
            break;
        case tt_literal_char:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;

            n.as_literal_expr->value.kind = ev_int;
            n.as_literal_expr->value.as_int = char_lit_value(p);

            advance_token;
            break;
        case tt_literal_float:
        case tt_literal_int:
        case tt_literal_string:
            TODO("simple literals");

            break;
        case tt_open_brace:
            TODO("compound literal");
            break;
        case tt_identifier:
        case tt_identifier_discard:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node(&p->alloca, astype_identifier_expr);
            n.as_identifier_expr->base.end = &current_token;
            n.as_identifier_expr->base.start = &current_token;
            n.as_identifier_expr->tok = &current_token;
            n.as_identifier_expr->is_discard = current_token.type == tt_identifier_discard;
            advance_token;
            break;

        case tt_open_paren:
            if (is_null_AST(n)) {
                // regular old paren expression
                n = new_ast_node(&p->alloca, astype_paren_expr);
                n.as_paren_expr->base.start = &current_token;
                advance_token;

                n.as_paren_expr->subexpr = parse_expr(p);
                if (current_token.type != tt_close_paren)
                    error_at_parser(p, "expected ')', got %s", token_type_str[current_token.type]);
                
                n.as_paren_expr->base.end = &current_token;
                advance_token;
            } else {
                // function call!
                AST temp;
                temp = new_ast_node(&p->alloca, astype_call_expr);
                temp.as_call_expr->base.start = n.base->start;
                advance_token;

                dynarr_init_AST(&temp.as_call_expr->params, 4);
                while (current_token.type != tt_close_paren) {
                    AST expr = parse_expr(p);
                    if (is_null_AST(expr)) {
                        error_at_parser(p, "expected expression");
                    }
                    dynarr_append_AST(&temp.as_call_expr->params, expr);
                    if (current_token.type == tt_comma) {
                        advance_token;
                        continue;
                    } else if (current_token.type == tt_close_paren) {
                        break;
                    } else {
                        error_at_parser(p, "expected ',' or ')'");
                    }
                }

                temp.as_call_expr->base.end = &current_token;
                advance_token;
                n = temp;
            }
            break;

        case tt_period:
            if (is_null_AST(n)) {
                // implicit selector
                n = new_ast_node(&p->alloca, astype_impl_selector_expr);
                
                n.as_impl_selector_expr->base.start = &current_token;
                advance_token;
                
                if (current_token.type != tt_identifier)
                    error_at_parser(p, "expected identifer after '.'");
                
                AST temp = new_ast_node(&p->alloca, astype_identifier_expr);

                token* ct = &current_token;
                temp.as_identifier_expr->base.end = ct;
                temp.as_identifier_expr->base.start = ct;
                temp.as_identifier_expr->tok = ct;

                n.as_impl_selector_expr->rhs = temp;

                n.as_impl_selector_expr->base.end = &current_token;
                advance_token;

            } else {
                // regular selector
                AST temp = new_ast_node(&p->alloca, astype_selector_expr);
                temp.as_selector_expr->base.start = n.base->start;
                advance_token;

                if (current_token.type != tt_identifier)
                    error_at_parser(p, "expected identifer after '.'");

                AST ident = new_ast_node(&p->alloca, astype_identifier_expr);
                
                token* ct = &current_token;
                ident.as_identifier_expr->base.end = ct;
                ident.as_identifier_expr->base.start = ct;
                ident.as_identifier_expr->tok = ct;
                temp.as_selector_expr->base.end = ct;

                temp.as_selector_expr->rhs = ident;
                temp.as_selector_expr->lhs = n;
                n = temp;
                advance_token;
            }
            break;

        case tt_colon_colon:
            // entity selection
            if (is_null_AST(n)) {
                error_at_parser(p, "expected expression before '::'");
            } else {
                AST temp = new_ast_node(&p->alloca, astype_entity_selector_expr);
                temp.as_entity_selector_expr->base.start = n.base->start;
                advance_token;

                if (current_token.type != tt_identifier)
                    error_at_parser(p, "expected identifer after '.'");

                AST ident = new_ast_node(&p->alloca, astype_identifier_expr);
                
                token* ct = &current_token;
                ident.as_identifier_expr->base.end = ct;
                ident.as_identifier_expr->base.start = ct;
                ident.as_identifier_expr->tok = ct;
                temp.as_entity_selector_expr->base.end = ct;

                temp.as_entity_selector_expr->rhs = ident;
                temp.as_entity_selector_expr->lhs = n;
                n = temp;
                advance_token;
            }
            break;

        case tt_open_bracket:

            if (is_null_AST(n))
                error_at_parser(p, "expected expression before slice/index expr (THIS SHOULD NEVER HAPPEN - CONTACT SANDWICH)");

            advance_token;

            AST first_expr = parse_expr(p);
            if (current_token.type == tt_close_bracket) {   // index expr
                if (is_null_AST(first_expr))
                    error_at_parser(p, "expected an expression inside '[]'");

                AST temp = new_ast_node(&p->alloca, astype_index_expr);
                temp.as_index_expr->base.start = n.base->start;
                temp.as_index_expr->base.end = &current_token;
                temp.as_index_expr->lhs = n;
                temp.as_index_expr->inside = first_expr;
                n = temp;
                advance_token;
            } else if (current_token.type == tt_colon) {    // slice expr
                advance_token;
                AST temp = new_ast_node(&p->alloca, astype_slice_expr);
                temp.as_slice_expr->base.start = n.base->start;
                temp.as_slice_expr->lhs = n;
                temp.as_slice_expr->inside_left = first_expr;

                temp.as_slice_expr->inside_right = parse_expr(p);
                if (current_token.type != tt_close_bracket)
                    error_at_parser(p, "expected closing ']'");

                temp.as_slice_expr->base.end = &current_token;
                n = temp;
                advance_token;
            } else {
                error_at_parser(p, "expected ':' or ']'");
            }

            break;
        
        case tt_carat: 
            if (is_null_AST(n))
                error_at_parser(p, "expected expression before deref (THIS SHOULD NEVER HAPPEN - CONTACT SANDWICH)");
            
            AST temp = new_ast_node(&p->alloca, astype_unary_op_expr);
            temp.as_unary_op_expr->base.start = n.base->start;
            temp.as_unary_op_expr->base.end = &current_token;
            temp.as_unary_op_expr->op = &current_token;
            temp.as_unary_op_expr->inside = n;

            n = temp;
            advance_token;
            break;

        case tt_type_keyword_addr:
        case tt_type_keyword_bool:
        case tt_type_keyword_int:
        case tt_type_keyword_i8:
        case tt_type_keyword_i16:
        case tt_type_keyword_i32:
        case tt_type_keyword_i64:
        case tt_type_keyword_uint:
        case tt_type_keyword_u8:
        case tt_type_keyword_u16:
        case tt_type_keyword_u32:
        case tt_type_keyword_u64:
        case tt_type_keyword_float:
        case tt_type_keyword_f16:
        case tt_type_keyword_f32:
        case tt_type_keyword_f64:
            TODO("simple type literals");
            break;
        
        case tt_keyword_struct:
        case tt_keyword_union:
        case tt_keyword_enum:
            TODO("complex type literals");
            break;

        default:
            out = true;
            break;
        }
    }
    return n;
}

AST parse_import_stmt(parser* restrict p) {
    TODO("parse_import_stmt()");
}

AST parse_stmt(parser* restrict p) {
    AST n = NULL_AST;

    switch (current_token.type) {
    case tt_keyword_defer:
        n = new_ast_node(&p->alloca, astype_defer_stmt);
        n.as_defer_stmt->base.start = &current_token;
        advance_token;

        n.as_defer_stmt->stmt = parse_stmt(p);

        n.as_defer_stmt->base.end = &peek_token(-1);
        break;
    
    case tt_keyword_if:
        n = new_ast_node(&p->alloca, astype_if_stmt);
        n.as_if_stmt->is_elif = false;
        n.as_if_stmt->base.start = &current_token;
        advance_token;

        n.as_if_stmt->condition = parse_expr(p);
        n.as_if_stmt->if_branch = parse_block_stmt(p);
        
        n.as_if_stmt->base.end = &peek_token(-1);

        if (current_token.type == tt_keyword_elif) {
            n.as_if_stmt->else_branch = parse_elif(p);
        }
        else if (current_token.type == tt_keyword_else) {
            advance_token;
            n.as_if_stmt->else_branch = parse_block_stmt(p);
        }
        break;
    
    case tt_keyword_while:
        n = new_ast_node(&p->alloca, astype_while_stmt);
        n.as_while_stmt->base.start = &current_token;
        advance_token;

        n.as_while_stmt->condition = parse_expr(p);
        if (n.as_while_stmt->condition.rawptr == NULL) {

        }
        n.as_while_stmt->block = parse_block_stmt(p);

        n.as_while_stmt->base.end = &peek_token(-1);
        break;
    
    case tt_keyword_for:
        // jank as shit but whatever, that's future sandwichman's problem
        if ((peek_token(1).type == tt_identifier || peek_token(1).type == tt_identifier_discard) &&
        (peek_token(2).type == tt_colon || peek_token(2).type == tt_keyword_in)) {
            // for-in loop!
            n = new_ast_node(&p->alloca, astype_for_in_stmt);
            n.as_for_in_stmt->base.start = &current_token;
            advance_token;

            n.as_for_in_stmt->indexvar = parse_expr(p);

            if (current_token.type == tt_colon) {
                advance_token;
                n.as_for_in_stmt->type = parse_expr(p);
                if (is_null_AST(n.as_for_in_stmt->type))
                    error_at_parser(p, "expected type expression after ':'");
            }

            if (current_token.type != tt_keyword_in)
                error_at_parser(p, "expected 'in', got '%s'", token_type_str[current_token.type]);
            advance_token;

            // catch #reverse
            if (current_token.type == tt_hash) {
                advance_token;
                if (string_eq(current_token.text, to_string("reverse"))) {
                    n.as_for_in_stmt->is_reverse = true;
                    advance_token;
                } else {
                    error_at_parser(p, "invalid directive \'%s\' in for-in loop", clone_to_cstring(current_token.text));
                }
            }


            n.as_for_in_stmt->from = parse_expr(p);
            if (current_token.type == tt_range_less) {
                n.as_for_in_stmt->is_inclusive = false;
            } else if (current_token.type == tt_range_eq) {
                n.as_for_in_stmt->is_inclusive = true;
            } else {
                error_at_parser(p, "expected '..<' or '..=', got '%s'", token_type_str[current_token.type]);
            }
            advance_token;
            
            n.as_for_in_stmt->to = parse_expr(p);
            n.as_for_in_stmt->block = parse_block_stmt(p);
            
            n.as_for_in_stmt->base.end = &peek_token(-1);
        } else {
            // regular for loop!
            TODO("regular for loop parsing");
            n = new_ast_node(&p->alloca, astype_for_stmt);
            n.as_for_stmt->base.start = &current_token;
            advance_token;

            n.as_for_stmt->prelude = parse_stmt(p);
            
            n.as_for_stmt->condition = parse_expr(p);
            if (current_token.type != tt_semicolon)
                error_at_parser(p, "expected ';', got '%s'", token_type_str[current_token.type]);

            advance_token;
            n.as_for_stmt->post_stmt = parse_stmt(p);
            
            n.as_for_stmt->block = parse_block_stmt(p);

            n.as_for_stmt->base.end = &peek_token(-1);
        }
        break;
    
    case tt_semicolon:
        n = new_ast_node(&p->alloca, astype_empty_stmt);
        n.as_empty_stmt->base.start = &current_token;
        n.as_empty_stmt->base.end = &current_token;
        break;
    
    case tt_open_bracket:
        n = parse_block_stmt(p);
        break;
    
    case tt_keyword_type:
        n = new_ast_node(&p->alloca, astype_type_decl_stmt);
        n.as_type_decl_stmt->base.start = &current_token;

        advance_token;
        if (current_token.type != tt_identifier)
            error_at_parser(p, "expected identifier, got '%s'", token_type_str[current_token.type]);

        n.as_type_decl_stmt->lhs = parse_expr(p);
        advance_token;
        
        if (current_token.type != tt_equal)
            error_at_parser(p, "expected '=', got '%s'", token_type_str[current_token.type]);
        advance_token;

        n.as_type_decl_stmt->rhs = parse_expr(p);
        if (is_null_AST(n.as_type_decl_stmt->rhs))
            error_at_parser(p, "expected type expression");

        if (current_token.type != tt_semicolon)
            error_at_parser(p, "expected ';' after type declaration", token_type_str[current_token.type]);
        
        n.as_type_decl_stmt->base.end = &current_token;
        advance_token;
        break;
    
    case tt_keyword_let:
    case tt_keyword_mut:
        n = new_ast_node(&p->alloca, astype_decl_stmt);
        n.as_decl_stmt->base.start = &current_token;
        n.as_decl_stmt->is_mut = (current_token.type == tt_keyword_mut);
        advance_token;

        // maybe directives?
        if (current_token.type == tt_hash) {
            advance_token;
            if (string_eq(current_token.text, to_string("static"))) {
                n.as_decl_stmt->is_static = true;
            } else if (string_eq(current_token.text, to_string("volatile"))) {
                n.as_decl_stmt->is_volatile = true;
            } else {
                error_at_parser(p, "invalid directive \'%s\' in declaration", clone_to_cstring(current_token.text));
            }
        }

        // parse identifier list
        dynarr_init_AST(&n.as_decl_stmt->lhs, 4); // four is probably good right
        while (current_token.type != tt_identifier || current_token.type != tt_identifier_discard) {
            dynarr_append_AST(&n.as_decl_stmt->lhs, parse_expr(p));
            if (current_token.type != tt_comma)
                break;
            advance_token;
        }

        // maybe types?
        if (current_token.type == tt_colon) {
            n.as_decl_stmt->has_expl_type = true;
            advance_token;
            n.as_decl_stmt->type = parse_expr(p);
            if (is_null_AST(n.as_decl_stmt->type))
                error_at_parser(p, "expected type expression");
        }

        // implicit initialization ONLY if a type has been provided
        if (current_token.type == tt_semicolon) {
            if (!n.as_decl_stmt->has_expl_type)
                error_at_parser(p, "cannot implicitly initialize without a type");
            break;
        }

        if (current_token.type != tt_equal)
            error_at_parser(p, "expected '=', got '%s'", token_type_str[current_token.type]);
        
        advance_token;

        if (current_token.type == tt_uninit) {
            n.as_decl_stmt->is_uninit = true;
            if (!n.as_decl_stmt->has_expl_type)
                error_at_parser(p, "uninitialization requires an explicit type");
            advance_token;
        } else {
            n.as_decl_stmt->rhs = parse_expr(p);
        }
        
        if (current_token.type != tt_semicolon)
            error_at_parser(p, "expected ';', got '%s'", token_type_str[current_token.type]);

        n.as_decl_stmt->base.end = &current_token;
        advance_token;
        break;
    
    default:
        TODO("parse_stmt() unimplemented");
        break;
    }
    return n;
}

// only for use within parse_stmt
AST parse_elif(parser* restrict p) {
    AST n = new_ast_node(&p->alloca, astype_if_stmt);
    n.as_if_stmt->is_elif = true;
    n.as_if_stmt->base.start = &current_token;

    advance_token;

    // get condition expression
    n.as_if_stmt->condition = parse_expr(p);
        
    // get block
    n.as_if_stmt->if_branch = parse_block_stmt(p);

    n.as_if_stmt->base.end = &peek_token(-1);

    if (current_token.type == tt_keyword_elif) {
        n.as_if_stmt->else_branch = parse_elif(p);
    }
    else if (current_token.type == tt_keyword_else) {
        advance_token;
        n.as_if_stmt->else_branch = parse_block_stmt(p);
    }
    return n;
}

AST parse_block_stmt(parser* restrict p) {
    AST n = new_ast_node(&p->alloca, astype_block_stmt);

    if (current_token.type != tt_open_brace)
        error_at_parser(p, "expected '{', got '%s'", token_type_str[current_token.type]);
    n.as_block_stmt->base.start = &current_token;

    advance_token;
    while (current_token.type != tt_close_brace) {
        AST stmt = parse_stmt(p);
        dynarr_append(AST, &n.as_block_stmt->stmts, stmt);
    }

    if (current_token.type != tt_close_brace)
        error_at_parser(p, "expected '}', got '%s'", token_type_str[current_token.type]);
    n.as_block_stmt->base.end = &current_token;

    return n;
}