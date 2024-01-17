#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

#define current_token ((p)->tokens.raw[(p)->current_tok])
#define peek_token(n) ((p)->tokens.raw[(p)->current_tok + (n)])
#define advance_token (((p)->current_tok + 1 < (p)->tokens.len) ? ((p)->current_tok)++ : 0)
#define advance_n_tok(n) (((p)->current_tok + n < (p)->tokens.len) ? ((p)->current_tok)+=n : 0)

#define str_from_tokens(start, end) ((string){(start).text.raw, (end).text.raw - (start).text.raw + (end).text.len})

#define error_at_parser(p, message, ...) \
    error_at_string((p)->path, (p)->src, current_token.text, message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token_index(p, index, message, ...) \
    error_at_string((p)->path, (p)->src, (p)->tokens.raw[index].text, message __VA_OPT__(,) __VA_ARGS__)

#define error_at_token(p, token, message, ...) \
    error_at_string((p)->path, (p)->src, (token).text, message __VA_OPT__(,) __VA_ARGS__)

#define error_at_AST(p, node, message, ...) \
    error_at_string((p)->path, (p)->src, str_from_tokens(*((node).base->start), *((node).base->end)), message __VA_OPT__(,) __VA_ARGS__)

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
    dump_tree(smth, 0);
}

AST parse_expr(parser* restrict p) {
    AST n = NULL_AST;
    n = parse_binary_expr(p, 0);
    return n;
}

int op_precedence(token_type type) {
    switch (type) {
    case tt_mul:
    case tt_div:
    case tt_mod:
    case tt_mod_mod:
    case tt_and:
    case tt_lshift:
    case tt_rshift:
    case tt_nor:
        return 5;
    case tt_add:
    case tt_sub:
    case tt_or:
    case tt_tilde:
        return 4;
    case tt_equal_equal:
    case tt_not_equal:
    case tt_less_equal:
    case tt_less_than:
    case tt_greater_equal:
    case tt_greater_than:
        return 3;
    case tt_and_and:
        return 2;
    case tt_or_or:
    case tt_tilde_tilde:
        return 1;
    }
    return 0;
}

AST parse_binary_expr(parser* restrict p, int precedence) {
    AST lhs = NULL_AST;
    
    lhs = parse_unary_expr(p, false);
    if (is_null_AST(lhs)) return lhs;

    while (precedence < op_precedence(current_token.type)) {
        lhs = parse_non_unary_expr(p, lhs, precedence);
    }

    return lhs;
}

AST parse_non_unary_expr(parser* restrict p, AST lhs, int precedence) {
    AST n = new_ast_node(&p->alloca, astype_binary_op_expr);
    n.as_binary_op_expr->base.start = lhs.base->start;
    n.as_binary_op_expr->op = &current_token;
    n.as_binary_op_expr->lhs = lhs;
    advance_token;
    
    n.as_binary_op_expr->rhs = parse_binary_expr(p, op_precedence(n.as_binary_op_expr->op->type));
    if (is_null_AST(n.as_binary_op_expr->rhs)) {
        error_at_parser(p, "expected operand");
    }
    n.as_binary_op_expr->base.end = n.as_binary_op_expr->rhs.base->end;
    return n;
}

AST parse_unary_expr(parser* restrict p, bool type_expr) {
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

        n.as_unary_op_expr->inside = parse_unary_expr(p, type_expr);

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

        n.as_cast_expr->rhs = parse_unary_expr(p, type_expr);

        n.as_cast_expr->base.end = &peek_token(-1);
        break;

    case tt_carat:
        n = new_ast_node(&p->alloca, astype_pointer_type_expr);
        n.as_pointer_type_expr->base.start = &current_token;
        advance_token;

        n.as_pointer_type_expr->subexpr = parse_unary_expr(p, type_expr);

        n.as_pointer_type_expr->base.end = &peek_token(-1);
        break;
    
    case tt_open_bracket:
        if (peek_token(1).type == tt_close_bracket) {
            n = new_ast_node(&p->alloca, astype_slice_type_expr);
            n.as_slice_type_expr->base.start = &current_token;

            advance_n_tok(2);
            n.as_slice_type_expr->subexpr = parse_unary_expr(p, type_expr);

            n.as_slice_type_expr->base.end = &peek_token(-1);
        } else {
            n = new_ast_node(&p->alloca, astype_array_type_expr);
            n.as_array_type_expr->base.start = &current_token;
            advance_token;

            n.as_array_type_expr->length = parse_expr(p);
            if (current_token.type != tt_close_bracket)
                error_at_parser(p, "expected ']', got %s", token_type_str[current_token.type]);

            advance_token;
            n.as_array_type_expr->subexpr = parse_unary_expr(p, type_expr);
            n.as_array_type_expr->base.end = &peek_token(-1);
        }
        break;

    default:
        n = parse_atomic_expr(p, type_expr);
    }
    return n;
}

i64 ascii_to_digit_val(parser* restrict p, char c, u8 base) {
    char val = base;
    if (c >= '0' && c <= '9') val = c-'0';
    if (c >= 'a' && c <= 'f') val = c-'a' +10;
    if (c >= 'A' && c <= 'F') val = c-'A' +10;
    
    if (val >= base)
        error_at_parser(p, "invalid base %d digit '%c'", base, c);
    return val;
}

i64 char_lit_value(parser* restrict p) {
    string t = current_token.text;

    if (t.raw[1] != '\\') { // trivial case
        if (t.len > 3) error_at_parser(p, "char literal too long");
        return t.raw[2];
    }

    switch(t.raw[2]) {
    case '0':  return 0;
    case 'a':  return '\a';
    case 'b':  return '\b';
    case 'e':  return 27;
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

f64 float_lit_value(parser* restrict p) {
    string t = current_token.text;
    f64 val = 0;

    bool is_negative = false;
    int digit_start = 0;

    if (t.raw[0] == '-') {
        is_negative = true;
        digit_start = 1;
    }

    u32 decimal_index = 0;
    for (u32 i = digit_start; i < t.len; i++) {
        if (t.raw[i] == '.') {
            decimal_index = i;
            break;
        }
        val = val*10 + (f64)ascii_to_digit_val(p, t.raw[i], 10);
    }

    u32 e_index = -1;
    f64 factor = 0.1;
    for (u32 i = decimal_index+1; i < t.len; i++) {
        if (t.raw[i] == 'e') {
            e_index = i;
            break;
        }
        val = val + ascii_to_digit_val(p, t.raw[i], 10) * factor;
        factor *= 0.1;
    }

    if (e_index == -1) return val;
    int exp_val = 0;
    for (int i = e_index+1; i < t.len; i++) {
        exp_val = exp_val*10 + ascii_to_digit_val(p, t.raw[i], 10);
    }

    val = val * pow(10.0, exp_val);

    return val * (is_negative ? -1 : 1);
}

i64 int_lit_value(parser* restrict p) {
    string t = current_token.text;
    i64 val = 0;

    bool is_negative = false;
    int digit_start = 0;

    if (t.raw[0] == '-') {
        is_negative = true;
        digit_start = 1;
    }

    if (t.raw[digit_start] != '0') { // basic base-10 parse
        FOR_URANGE_EXCL(i, digit_start, t.len) {
            val = val * 10 + ascii_to_digit_val(p, t.raw[i], 10);
        }
        return val * (is_negative ? -1 : 1);
    }
    
    if (t.len == 1) return 0; // simple '0'
    if (is_negative && t.len == 2) return 0; // simple '-0'

    u8 base = 10;
    switch (t.raw[digit_start+1]) {
    case 'x':
    case 'X':
        base = 16; break;
    case 'o':
    case 'O':
        base = 8; break;
    case 'b':
    case 'B':
        base = 2; break;
    case 'd':
    case 'D':
        break;
    default:
        error_at_parser(p, "invalid base prefix '%c'", t.raw[digit_start+1]);
    }

    if (t.len < 3 + digit_start) error_at_parser(p, "expected digit after '%c'", t.raw[digit_start+1]);

    FOR_URANGE_EXCL(i, 2 + digit_start, t.len) {
        val = val * 10 + ascii_to_digit_val(p, t.raw[i], base);
    }
    return val * (is_negative ? -1 : 1);
}

string string_lit_value(parser* restrict p) {
    string t = current_token.text;
    string val = NULL_STR;
    size_t val_len = 0;

    // trace string, figure out how long it needs to be
    FOR_URANGE_EXCL(i, 1, t.len-1) {
        if (t.raw[i] != '\\') {
            val_len++;
            continue;
        }
        i++;
        switch (t.raw[i]) {
        case 'x':
            i++;
            i++;
        case '0':
        case 'a':
        case 'b':
        case 'e':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\\':
        case '\"':
        case '\'':
            val_len++;
            break;
        default:
            error_at_parser(p, "invalid escape sequence '\\%c'", t.raw[i]);
            break;
        }
    }

    // allocate
    val = (string){arena_alloc(&p->alloca, val_len, 1), val_len};

    // fill in string with correct bytes
    u64 val_i = 0;
    FOR_URANGE_EXCL(i, 1, t.len-1) {
        if (t.raw[i] != '\\') {
            val.raw[val_i] = t.raw[i];
            val_i++;
            continue;
        }
        i++;
        switch (t.raw[i]) {
        case '0': val.raw[val_i] = '\0'; break;
        case 'a': val.raw[val_i] = '\a'; break;
        case 'b': val.raw[val_i] = '\b'; break;
        case 'e': val.raw[val_i] = '\e'; break;
        case 'f': val.raw[val_i] = '\f'; break;
        case 'n': val.raw[val_i] = '\n'; break;
        case 'r': val.raw[val_i] = '\r'; break;
        case 't': val.raw[val_i] = '\t'; break;
        case 'v': val.raw[val_i] = '\v'; break;
        case '\\': val.raw[val_i] = '\\'; break;
        case '\"': val.raw[val_i] = '\"'; break;
        case '\'': val.raw[val_i] = '\''; break;
        case 'x':
            val.raw[val_i] = ascii_to_digit_val(p, t.raw[i+1], 16) * 0x10 + ascii_to_digit_val(p, t.raw[i+2], 16);
            i += 2;
            break;
        default:
            error_at_parser(p, "invalid escape sequence '\\%c'", t.raw[i]);
            break;
        }
        val_i++;
    }


    return val;
}

// takes a parsed AST node and determines if it could possibly be a type expession
bool is_possible_type_expr(AST n) {
    switch (n.type) {
    case astype_array_type_expr:
    case astype_slice_type_expr:
    case astype_pointer_type_expr:
    case astype_struct_type_expr:
    case astype_union_type_expr:
    case astype_enum_type_expr:
    case astype_fn_type_expr:

    case astype_identifier_expr:
    case astype_basic_type_expr:
        return true;
    default:
        return false;
    }
}

#define is_definitely_not_type_expr_trust_me_bro(ast_node) (!is_possible_type_expr((ast_node)))

// jesus christ
AST parse_atomic_expr(parser* restrict p, bool type_expr) {
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

            n.as_literal_expr->value.kind = ev_pointer; 
            n.as_literal_expr->value.as_pointer = 0;

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
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;
        
            n.as_literal_expr->value.kind = ev_float;
            n.as_literal_expr->value.as_float = float_lit_value(p);

            advance_token;
            break;
        case tt_literal_int:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;
        
            n.as_literal_expr->value.kind = ev_int;
            n.as_literal_expr->value.as_int = int_lit_value(p);

            advance_token;
            break;
        case tt_literal_string:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;
        
            n.as_literal_expr->value.kind = ev_string;
            n.as_literal_expr->value.as_string = string_lit_value(p);

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
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node(&p->alloca, astype_basic_type_expr);
            n.as_basic_type_expr->base.start = &current_token;
            n.as_basic_type_expr->base.end = &current_token;
            n.as_basic_type_expr->lit = &current_token;

            advance_token;
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

                temp.as_call_expr->lhs = n;
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

                temp.as_identifier_expr->base.end = &current_token;
                temp.as_identifier_expr->base.start = &current_token;
                temp.as_identifier_expr->tok = &current_token;

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
                
                ident.as_identifier_expr->base.end = &current_token;
                ident.as_identifier_expr->base.start = &current_token;
                ident.as_identifier_expr->tok = &current_token;
                temp.as_selector_expr->base.end = &current_token;

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
                
                ident.as_identifier_expr->base.end = &current_token;
                ident.as_identifier_expr->base.start = &current_token;
                ident.as_identifier_expr->tok = &current_token;
                temp.as_entity_selector_expr->base.end = &current_token;

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

        case tt_keyword_struct:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node(&p->alloca, astype_struct_type_expr);

            n.base->start = &current_token;
            advance_token;

            if (current_token.type != tt_open_brace)
                error_at_parser(p, "expected '{' after 'struct'");
            advance_token;

            if (current_token.type == tt_close_brace) {
                dynarr_init_AST_typed_field(&n.as_struct_type_expr->fields, 1);
                advance_token;
                break;
            }

            dynarr_init_AST_typed_field(&n.as_struct_type_expr->fields, 8);
            
            while (true) {

                AST field = parse_expr(p);

                if (is_null_AST(field))
                    error_at_parser(p, "expected struct name");
                if (field.type != astype_identifier_expr)
                    error_at_AST(p, field, "field must be an identifier");
                
                AST type = NULL_AST;
                if (current_token.type == tt_colon) {
                    advance_token;
                    type = parse_expr(p);

                    if (current_token.type == tt_semicolon) {
                        dynarr_append_AST_typed_field(&n.as_struct_type_expr->fields, (AST_typed_field){
                            field, type
                        });
                        advance_token;
                        if (current_token.type == tt_close_brace) {
                            break;
                        }
                        continue;
                    } else {
                        error_at_parser(p, "expected ';' after field definition");
                    }

                } else if (current_token.type == tt_comma) {
                    dynarr_append_AST_typed_field(&n.as_struct_type_expr->fields, (AST_typed_field){
                        field, type
                    });
                    advance_token;
                    if (current_token.type != tt_identifier) {
                        error_at_parser(p, "expected field name");
                    }
                    continue;
                } else {
                    error_at_parser(p, "expected ':' or ','");
                }
            }
            n.base->end = &current_token;
            advance_token;
            break;

        case tt_keyword_union:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node(&p->alloca, astype_union_type_expr);

            n.base->start = &current_token;
            advance_token;

            if (current_token.type != tt_open_brace)
                error_at_parser(p, "expected '{' after 'struct'");
            advance_token;

            if (current_token.type == tt_close_brace) {
                dynarr_init_AST_typed_field(&n.as_union_type_expr->fields, 1);
                advance_token;
                break;
            }

            dynarr_init_AST_typed_field(&n.as_union_type_expr->fields, 8);
            
            while (true) {

                AST field = parse_expr(p);

                if (is_null_AST(field))
                    error_at_parser(p, "expected field name");
                if (field.type != astype_identifier_expr)
                    error_at_AST(p, field, "field must be an identifier");
                
                AST type = NULL_AST;
                if (current_token.type == tt_colon) {
                    advance_token;
                    type = parse_expr(p);

                    if (current_token.type == tt_semicolon) {
                        dynarr_append_AST_typed_field(&n.as_struct_type_expr->fields, (AST_typed_field){
                            field, type
                        });
                        advance_token;
                        if (current_token.type == tt_close_brace) {
                            break;
                        }
                        continue;
                    } else {
                        error_at_parser(p, "expected ';' after field declaration");
                    }

                } else if (current_token.type == tt_comma) {
                    dynarr_append_AST_typed_field(&n.as_union_type_expr->fields, (AST_typed_field){
                        field, type
                    });
                    advance_token;
                    if (current_token.type != tt_identifier) {
                        error_at_parser(p, "expected field name");
                    }
                    continue;
                } else {
                    error_at_parser(p, "expected ':' or ','");
                }
            }
            n.base->end = &current_token;
            advance_token;
            break;

        case tt_keyword_enum:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node(&p->alloca, astype_enum_type_expr);
            n.base->start = &current_token;
            advance_token;


            // parse optional backing type
            if (current_token.type != tt_open_brace) {
                AST enum_type = parse_type_expr(p);
                if (is_null_AST(enum_type)) {
                    error_at_parser(p, "expected a type");
                }
                n.as_enum_type_expr->backing_type = enum_type;
            }

            if (current_token.type != tt_open_brace)
                error_at_parser(p, "expected '{'");
            advance_token;



            if (current_token.type == tt_close_brace) {
                dynarr_init_AST_enum_variant(&n.as_enum_type_expr->variants, 1);
                n.base->end = &current_token;
                advance_token;
                break;
            } else {
                dynarr_init_AST_enum_variant(&n.as_enum_type_expr->variants, 8); // 8 is probably an ok size
            }

            u64 value = 0;
            while (true) {

                AST variant = parse_expr(p);
                if (is_null_AST(variant))
                    error_at_parser(p, "expected field name");
                if (variant.type != astype_identifier_expr)
                    error_at_AST(p, variant, "enum variant must be an identifier");
                

                if (current_token.type == tt_equal) {
                    advance_token;
                    if (current_token.type != tt_literal_int)
                        error_at_parser(p, "enum variant value must be an integer literal");

                    value = int_lit_value(p);
                    advance_token;
                }

                dynarr_append_AST_enum_variant(&n.as_enum_type_expr->variants, (AST_enum_variant){
                    variant,
                    value
                });
                
                value++;

                if (current_token.type == tt_comma) {
                    advance_token;
                    if (current_token.type == tt_close_brace) {
                        break;
                    } else {
                        continue;
                    }
                } else if (current_token.type == tt_close_brace) {
                    break;
                } else {
                    error_at_parser(p, "expected ',' or '}'");
                }

            }

            n.base->end = &current_token;
            advance_token;
            break;

        case tt_keyword_fn:
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node(&p->alloca, astype_fn_type_expr);
            n.base->start = &current_token;
            advance_token;

            if (current_token.type != tt_open_paren)
                error_at_parser(p, "expected '(' after 'fn'");
            advance_token;


            dynarr_init_AST_typed_field(&n.as_fn_type_expr->parameters, 4);
            dynarr_init_AST_typed_field(&n.as_fn_type_expr->returns, 2);
            
            while (current_token.type != tt_close_paren) {

                AST field = parse_expr(p);

                if (is_null_AST(field))
                    error_at_parser(p, "expected parameter name");
                if (field.type != astype_identifier_expr)
                    error_at_AST(p, field, "parameter must be an identifier");
                
                AST type = NULL_AST;
                if (current_token.type == tt_colon) {
                    advance_token;
                    type = parse_expr(p);

                    if (current_token.type == tt_comma) {
                        dynarr_append_AST_typed_field(&n.as_fn_type_expr->parameters, (AST_typed_field){
                            field, type
                        });
                        advance_token;
                        if (current_token.type == tt_close_paren) {
                            break;
                        }
                        continue;
                    } else if (current_token.type == tt_close_paren) {
                        break;
                    } else {
                        error_at_parser(p, "expected ',' or ')'");
                    }

                } else if (current_token.type == tt_comma) {
                    dynarr_append_AST_typed_field(&n.as_fn_type_expr->parameters, (AST_typed_field){
                        field, type
                    });
                    advance_token;
                    if (current_token.type != tt_identifier) {
                        error_at_parser(p, "expected parameter name");
                    }
                    continue;
                } else {
                    error_at_parser(p, "expected ':' or ','");
                }
            }
            n.base->end = &current_token;
            advance_token;

            // no return type
            if (current_token.type != tt_arrow_right) {
                break;
            }
            advance_token;

            // simple return type (single, untyped value)
            if (current_token.type != tt_open_paren) {
                AST return_type = parse_type_expr(p);
                if (is_null_AST(return_type))
                    error_at_parser(p, "expected a type expression");
                if (!is_possible_type_expr(return_type))
                    error_at_AST(p, return_type, "return type must be a type expression");

                n.as_fn_type_expr->simple_return = true;
                dynarr_append_AST_typed_field(&n.as_fn_type_expr->returns, (AST_typed_field){
                    NULL_AST, return_type
                });
                n.base->end = &peek_token(-1);
                break;
            }

            // else, complex return
            advance_token;
            while (current_token.type != tt_close_paren) {

                AST field = parse_expr(p);

                if (is_null_AST(field))
                    error_at_parser(p, "expected return variable name");
                if (field.type != astype_identifier_expr)
                    error_at_AST(p, field, "return variable must be an identifier");
                
                AST type = NULL_AST;
                if (current_token.type == tt_colon) {
                    advance_token;
                    type = parse_expr(p);

                    if (current_token.type == tt_comma) {
                        dynarr_append_AST_typed_field(&n.as_fn_type_expr->returns, (AST_typed_field){
                            field, type
                        });
                        advance_token;
                        if (current_token.type == tt_close_paren) {
                            break;
                        }
                        continue;
                    } else if (current_token.type == tt_close_paren) {
                        break;
                    } else {
                        error_at_parser(p, "expected ',' or ')'");
                    }

                } else if (current_token.type == tt_comma) {
                    dynarr_append_AST_typed_field(&n.as_fn_type_expr->returns, (AST_typed_field){
                        field, type
                    });
                    advance_token;
                    if (current_token.type != tt_identifier) {
                        error_at_parser(p, "expected return variable name");
                    }
                    continue;
                } else {
                    error_at_parser(p, "expected ':' or ','");
                }
            }
            n.base->end = &current_token;
            advance_token;

            break;

        case tt_open_brace:

            // if we are looking for a type expression, return the previously parsed expression
            // because this aint gonna be a type
            if (type_expr) {
                out = true;
                break;
            }

            // if previously parsed expression exists and isnt a type, return that expression
            if (!is_null_AST(n) && is_definitely_not_type_expr_trust_me_bro(n)) {
                out = true;
                break;
            }

            if (n.type == astype_fn_type_expr) {
                AST func_lit = new_ast_node(&p->alloca, astype_func_literal_expr);
                func_lit.as_func_literal_expr->base.start = n.base->start;
                func_lit.as_func_literal_expr->type = n;
                AST code_block = parse_block_stmt(p);
                func_lit.as_func_literal_expr->base.end = code_block.base->end;
                n = func_lit;
                break;
            }

            AST lit = new_ast_node(&p->alloca, astype_comp_literal_expr);
            lit.as_comp_literal_expr->type = n;

            if (!is_null_AST(n)) {
                lit.base->start = n.base->start;
            } else {
                lit.base->start = &current_token;
            }

            advance_token;
            if (current_token.type == tt_close_brace) {
                dynarr_init_AST(&lit.as_comp_literal_expr->elems, 1);
                lit.base->end = &current_token;
                break;
            }

            dynarr_init_AST(&lit.as_comp_literal_expr->elems, 4);

            while (true) {
                AST expr = parse_expr(p);

                if (is_null_AST(expr))
                    error_at_parser(p, "expected an expression");

                dynarr_append_AST(&lit.as_comp_literal_expr->elems, expr);

                if (current_token.type == tt_comma) {
                    advance_token;
                    if (current_token.type == tt_close_brace) {
                        break;
                    } else {
                        continue;
                    }
                } else if (current_token.type == tt_close_brace) {
                    break;
                } else {
                    error_at_parser(p, "expected ',' or '}'");
                }
            }

            // TODO("enum shittery");
            lit.base->end = &current_token;
            n = lit;
            advance_token;
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
            // TODO("regular for loop parsing");
            n = new_ast_node(&p->alloca, astype_for_stmt);
            n.as_for_stmt->base.start = &current_token;
            advance_token;

            n.as_for_stmt->prelude = parse_stmt(p);
            
            n.as_for_stmt->condition = parse_expr(p);
            if (is_null_AST(n.as_for_stmt->condition))
                error_at_parser(p, "expected an expression", token_type_str[current_token.type]);

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
        TODO("parse_stmt(): unimplemented");
        // todo:
        // - assignment statement
        // - compound assignment statement
        // - import statement
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
    advance_token;
    return n;
}