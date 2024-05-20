#include "orbit.h"
#include "term.h"

#include "parse.h"
#include "ast.h"

// sandwichman's BLAZINGLY 🔥🔥 FAST 🚀🚀 parser in RUST 🦀🦀 + AI POWERED with ChatGPT 5.0 🤖🧠 and BLOCKCHAIN NFT ETHEREUM WEB3 TECHNOLOGY

// construct a parser struct from a lexer and an arena allocator
parser make_parser(lexer* l, arena* alloca) {
    parser p = {0};
    p.alloca = alloca;
    p.tokens = l->buffer;
    p.path   = l->path;
    p.src    = l->src;
    p.current_tok = 0;
    p.num_nodes = 0;
    return p;
}

AST parse_module_decl(parser* p) {

    AST n = new_ast_node_p(p, AST_module_decl);

    if (current_token.type != TOK_KEYWORD_MODULE)
        error_at_parser(p, "expected module declaration");

    n.as_module_decl->base.start = &current_token;

    advance_token;
    if (current_token.type != TOK_IDENTIFIER)
        error_at_parser(p, "expected module name");

    n.as_module_decl->name = &current_token;

    advance_token;
    if (current_token.type != TOK_SEMICOLON)
        error_at_parser(p, "expected ';'");
    
    n.as_module_decl->base.end = &current_token;
    advance_token;
    return n;
}

void parse_file(parser* p) {
    
    p->module_decl = parse_module_decl(p);

    da_init(&p->stmts, 16);

    while (current_token.type != TOK_EOF) {
        AST stmt = parse_stmt(p);
        if (is_null_AST(stmt)) break;
        da_append(&p->stmts, stmt);
    }
}

int op_precedence(token_type type) {
    switch (type) {
    case TOK_MUL:
    case TOK_DIV:
    case TOK_MOD:
    case TOK_MOD_MOD:
    case TOK_AND:
    case TOK_LSHIFT:
    case TOK_RSHIFT:
    case TOK_NOR:
        return 5;
    case TOK_ADD:
    case TOK_SUB:
    case TOK_OR:
    case TOK_TILDE:
        return 4;
    case TOK_EQUAL_EQUAL:
    case TOK_NOT_EQUAL:
    case TOK_LESS_EQUAL:
    case TOK_LESS_THAN:
    case TOK_GREATER_EQUAL:
    case TOK_GREATER_THAN:
        return 3;
    case TOK_AND_AND:
        return 2;
    case TOK_OR_OR:
    // case TOK_TILDE_TILDE:
        return 1;
    default:
        return 0;
    }
}

AST parse_binary_expr(parser* p, int precedence, bool no_cl) {
    AST lhs = NULL_AST;
    
    lhs = parse_unary_expr(p, no_cl);
    if (is_null_AST(lhs)) return lhs;

    while (precedence < op_precedence(current_token.type)) {
        lhs = parse_non_unary_expr(p, lhs, precedence, no_cl);
    }

    return lhs;
}

AST parse_non_unary_expr(parser* p, AST lhs, int precedence, bool no_cl) {
    AST n = new_ast_node_p(p, AST_binary_op_expr);
    n.as_binary_op_expr->base.start = lhs.base->start;
    n.as_binary_op_expr->op = &current_token;
    n.as_binary_op_expr->lhs = lhs;
    advance_token;
    
    n.as_binary_op_expr->rhs = parse_binary_expr(p, op_precedence(n.as_binary_op_expr->op->type), no_cl);
    if (is_null_AST(n.as_binary_op_expr->rhs)) {
        error_at_parser(p, "expected operand");
    }
    n.as_binary_op_expr->base.end = n.as_binary_op_expr->rhs.base->end;
    return n;
}

AST parse_unary_expr(parser* p, bool no_cl) {
    AST n = NULL_AST;
    
    switch (current_token.type) {
    case TOK_SUB:
    case TOK_TILDE:
    case TOK_EXCLAM:
    case TOK_AND:
    case TOK_KEYWORD_SIZEOF:
    case TOK_KEYWORD_ALIGNOF:
    case TOK_KEYWORD_INLINE: {
        n = new_ast_node_p(p, AST_unary_op_expr);
        n.as_unary_op_expr->base.start = &current_token;
        n.as_unary_op_expr->op = &current_token;
        advance_token;

        n.as_unary_op_expr->inside = parse_unary_expr(p, no_cl);
        if (is_null_AST(n.as_unary_op_expr->inside)) {
            error_at_parser(p, "expected an expression");
        }

        n.as_unary_op_expr->base.end = &peek_token(-1);
    } break;
    case TOK_KEYWORD_CAST:
    case TOK_KEYWORD_BITCAST: {
        n = new_ast_node_p(p, AST_cast_expr);
        n.as_cast_expr->base.start = &current_token;
        n.as_cast_expr->is_bitcast = current_token.type == TOK_KEYWORD_BITCAST;
        advance_token;

        if (current_token.type != TOK_OPEN_PAREN)
            error_at_parser(p, "expected '(' after 'cast'");
        advance_token;

        n.as_cast_expr->type = parse_expr(p, false);

        if (current_token.type != TOK_CLOSE_PAREN)
            error_at_parser(p, "expected ')'");
        advance_token;

        n.as_cast_expr->rhs = parse_unary_expr(p, no_cl);
        if (is_null_AST(n.as_cast_expr->rhs)) {
            error_at_parser(p, "expected an expression");
        }

        n.as_cast_expr->base.end = &peek_token(-1);
    } break;
    case TOK_CARET: {
        n = new_ast_node_p(p, AST_pointer_type_expr);
        n.as_pointer_type_expr->base.start = &current_token;
        advance_token;

        if (current_token.type == TOK_KEYWORD_LET) {
            n.as_pointer_type_expr->mutable = false;
        } else if (current_token.type == TOK_KEYWORD_MUT) {
            n.as_pointer_type_expr->mutable = true;
        } else {
            error_at_parser(p, "pointer type must be 'mut' or 'let' (e.g. ^mut T)");
        }
        advance_token;

        n.as_pointer_type_expr->subexpr = parse_unary_expr(p, no_cl);

        n.as_pointer_type_expr->base.end = &peek_token(-1);
    } break;
    case TOK_KEYWORD_DISTINCT: {
        n = new_ast_node_p(p, AST_distinct_type_expr);
        n.as_distinct_type_expr->base.start = &current_token;
        advance_token;

        n.as_distinct_type_expr->subexpr = parse_unary_expr(p, no_cl);
        if (is_null_AST(n.as_distinct_type_expr->subexpr)) {
            error_at_parser(p, "expected an expression");
        }

        n.as_distinct_type_expr->base.end = &peek_token(-1);
    } break;
    case TOK_OPEN_BRACKET: {
        if (peek_token(1).type == TOK_CLOSE_BRACKET) {
            n = new_ast_node_p(p, AST_slice_type_expr);
            n.as_slice_type_expr->base.start = &current_token;
            advance_n_tok(2);

            if (current_token.type == TOK_KEYWORD_LET) {
                n.as_slice_type_expr->mutable = false;
            } else if (current_token.type == TOK_KEYWORD_MUT) {
                n.as_slice_type_expr->mutable = true;
            } else {
                error_at_parser(p, "slice type must be 'mut' or 'let' (e.g. []mut T)");
            }
            advance_token;

            n.as_slice_type_expr->subexpr = parse_unary_expr(p, no_cl);
            if (is_null_AST(n.as_slice_type_expr->subexpr)) {
                error_at_parser(p, "expected an expression");
            }

            n.as_slice_type_expr->base.end = &peek_token(-1);
        } else {
            n = new_ast_node_p(p, AST_array_type_expr);
            n.as_array_type_expr->base.start = &current_token;
            advance_token;

            n.as_array_type_expr->length = parse_expr(p, no_cl);
            if (current_token.type != TOK_CLOSE_BRACKET)
                error_at_parser(p, "expected ']'");

            advance_token;
            n.as_array_type_expr->subexpr = parse_unary_expr(p, no_cl);
            if (is_null_AST(n.as_array_type_expr->subexpr)) {
                error_at_parser(p, "expected an expression");
            }
            
            n.as_array_type_expr->base.end = &peek_token(-1);
        }
    } break;
    default:
        n = parse_atomic_expr(p, no_cl);
    }
    return n;
}

int ascii_to_digit_val(parser* p, char c, u8 base) {
    char val = (char)base;
    if (c >= '0' && c <= '9') val = (char)(c-'0');
    if (c >= 'a' && c <= 'f') val = (char)(c-'a' + 10);
    if (c >= 'A' && c <= 'F') val = (char)(c-'A' + 10);
    
    if (val >= base)
        error_at_parser(p, "invalid base %d digit '%c'", base, c);
    return val;
}

i64 char_lit_value(parser* p) {
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

f64 float_lit_value(parser* p) {
    string t = current_token.text;
    f64 val = 0;

    int digit_start = 0;

    int decimal_index = 0;
    for (int i = digit_start; i < t.len; i++) {
        if (t.raw[i] == '.') {
            decimal_index = i;
            break;
        }
        val = val*10 + (f64)ascii_to_digit_val(p, t.raw[i], 10);
    }

    int e_index = -1;
    f64 factor = 0.1;
    for (int i = decimal_index+1; i < t.len; i++) {
        if (t.raw[i] == 'e') {
            e_index = i;
            break;
        }
        val = val + (f64)ascii_to_digit_val(p, t.raw[i], 10) * factor;
        factor *= 0.1;
    }

    int exp_val = 0;

    if (e_index == -1) return val;
    if (t.raw[e_index+1] == '-') e_index++;
    
    for (int i = e_index+1; i < t.len; i++) {
        exp_val = exp_val*10 + ascii_to_digit_val(p, t.raw[i], 10);
    }

    val *= pow(10.0, (t.raw[e_index] == '-' ? -exp_val : exp_val));

    // printf("\n\n%lf\n\n", (is_negative ? -val : val));

    return val;
}

i64 int_lit_value(parser* p) {
    string t = current_token.text;
    i64 val = 0;

    bool is_negative = false;
    int digit_start = 0;

    if (t.raw[0] == '-') {
        is_negative = true;
        digit_start = 1;
    }

    if (t.raw[digit_start] != '0') { // basic base-10 parse
        for_urange(i, digit_start, t.len) {
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

    for_urange(i, 2 + digit_start, t.len) {
        val = val * base + ascii_to_digit_val(p, t.raw[i], base);
    }
    return val * (is_negative ? -1 : 1);
}

string string_lit_value(parser* p) {
    string t = current_token.text;
    string val = NULL_STR;
    size_t val_len = 0;

    // trace string, figure out how long it needs to be
    for_urange(i, 1, t.len-1) {
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
    val = (string){arena_alloc(p->alloca, val_len, 1), val_len};

    // fill in string with correct bytes
    u64 val_i = 0;
    for_urange(i, 1, t.len-1) {
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
    case AST_array_type_expr:
    case AST_slice_type_expr:
    case AST_pointer_type_expr:
    case AST_distinct_type_expr:
    case AST_struct_type_expr:
    case AST_union_type_expr:
    case AST_enum_type_expr:
    case AST_fn_type_expr:
    case AST_identifier_expr:
    case AST_basic_type_expr:
        return true;
    }
    return false;
}

#define is_definitely_not_type_expr_trust_me_bro(ast_node) (!is_possible_type_expr((ast_node)))

void parse_typed_field_list(parser* p, da(AST_typed_field)* list, token_type ending) {
    while (current_token.type != ending) {

        AST field = parse_expr(p, true);

        if (is_null_AST(field))
            error_at_parser(p, "expected identifier");
        if (field.type != AST_identifier_expr)
            error_at_AST(p, field, "expected identifier");
        
        AST type = NULL_AST;
        if (current_token.type == TOK_COLON) {
            advance_token;
            type = parse_expr(p, true);
            if (is_null_AST(type)) {
                error_at_parser(p, "expected type");
            }

            if (current_token.type == TOK_COMMA) {
                da_append(list, ((AST_typed_field){
                    field, type
                }));
                advance_token;
                    if (current_token.type == TOK_CLOSE_PAREN) {
                        da_append(list, ((AST_typed_field){
                        field, type
                    }));
                    break;
                }
                continue;
            } else if (current_token.type == TOK_CLOSE_PAREN) {
                da_append(list, ((AST_typed_field){
                    field, type
                }));
                break;
            } else {
                error_at_parser(p, "expected ',' or '%s'", token_type_str[ending]);
            }

        } else if (current_token.type == TOK_COMMA) {
            da_append(list, ((AST_typed_field){
                field, type
            }));
            advance_token;
            if (current_token.type != TOK_IDENTIFIER) {
                error_at_parser(p, "expected parameter name");
            }
            continue;
        } else {
            error_at_parser(p, "expected ':' or ','");
        }
    }
}

// jesus christ
AST parse_atomic_expr(parser* p, bool no_cl) {
    AST n = NULL_AST;

    bool out = false;
    while (!out) {
        switch (current_token.type) {
        case TOK_LITERAL_NULL: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;

            n.as_literal_expr->value.kind = EV_POINTER; 
            n.as_literal_expr->value.as_pointer = 0;
            n.as_literal_expr->value.freeable = false;

            advance_token;
        } break;
        case TOK_LITERAL_BOOL: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;

            n.as_literal_expr->value.kind = EV_BOOL;
            n.as_literal_expr->value.as_bool = string_eq(current_token.text, str("true"));
            n.as_literal_expr->value.freeable = false;

            advance_token;
        } break;
        case TOK_LITERAL_CHAR: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;

            n.as_literal_expr->value.kind = EV_UNTYPED_INT;
            n.as_literal_expr->value.as_untyped_int = char_lit_value(p);
            n.as_literal_expr->value.freeable = false;

            advance_token;
        } break;
        case TOK_LITERAL_FLOAT: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;
        
            n.as_literal_expr->value.kind = EV_UNTYPED_FLOAT;
            n.as_literal_expr->value.as_untyped_float = float_lit_value(p);
            n.as_literal_expr->value.freeable = false;

            advance_token;
        } break;
        case TOK_LITERAL_INT: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token;
            n.as_literal_expr->base.end = &current_token;
        
            n.as_literal_expr->value.kind = EV_UNTYPED_INT;
            n.as_literal_expr->value.as_untyped_int = int_lit_value(p);
            n.as_literal_expr->value.freeable = false;

            advance_token;
        } break;
        case TOK_LITERAL_STRING: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token;
        
            n.as_literal_expr->value.kind = EV_STRING;
            n.as_literal_expr->value.freeable = false;

            string value = NULL_STR;
            value = string_lit_value(p);
            advance_token;
            // this leaks memory but it kind of doesnt matter cause they're really tiny! 
            // otherwise the mars_alloc/mars_free overhead is pretty big
            while (current_token.type == TOK_LITERAL_STRING) {
                string str = string_lit_value(p);
                string new_value = (string){arena_alloc(p->alloca, value.len + str.len, 1), value.len + str.len};
                string_concat_buf(new_value, value, str);
                value = new_value;
                advance_token;
            }

            n.as_literal_expr->value.as_string = value;
            n.as_literal_expr->base.end = &peek_token(-1);

        } break;
        case TOK_TYPE_KEYWORD_BOOL:
        case TOK_TYPE_KEYWORD_INT:
        case TOK_TYPE_KEYWORD_I8:
        case TOK_TYPE_KEYWORD_I16:
        case TOK_TYPE_KEYWORD_I32:
        case TOK_TYPE_KEYWORD_I64:
        case TOK_TYPE_KEYWORD_UINT:
        case TOK_TYPE_KEYWORD_U8:
        case TOK_TYPE_KEYWORD_U16:
        case TOK_TYPE_KEYWORD_U32:
        case TOK_TYPE_KEYWORD_U64:
        case TOK_TYPE_KEYWORD_FLOAT:
        case TOK_TYPE_KEYWORD_F16:
        case TOK_TYPE_KEYWORD_F32:
        case TOK_TYPE_KEYWORD_F64: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }

            n = new_ast_node_p(p, AST_basic_type_expr);
            n.as_basic_type_expr->base.start = &current_token;
            n.as_basic_type_expr->base.end = &current_token;
            n.as_basic_type_expr->lit = &current_token;

            advance_token;
        } break;
        case TOK_IDENTIFIER:
        case TOK_IDENTIFIER_DISCARD: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node_p(p, AST_identifier_expr);
            n.as_identifier_expr->base.end = &current_token;
            n.as_identifier_expr->base.start = &current_token;
            n.as_identifier_expr->tok = &current_token;
            n.as_identifier_expr->is_discard = current_token.type == TOK_IDENTIFIER_DISCARD;
            advance_token;
        } break;
        case TOK_KEYWORD_OFFSETOF: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node_p(p, AST_binary_op_expr);
            n.as_binary_op_expr->base.start = &current_token;
            n.as_binary_op_expr->op = &current_token;
            advance_token;
            n.as_binary_op_expr->lhs = parse_expr(p, false);
            advance_token;
            if (current_token.type != TOK_COMMA) {
                error_at_parser(p, "expected ','");
            }
            advance_token;
            n.as_binary_op_expr->rhs = parse_expr(p, false);
            if (current_token.type == TOK_COMMA) advance_token;
            if (current_token.type != TOK_CLOSE_PAREN) {
                error_at_parser(p, "expected ')'");
            }
            n.as_binary_op_expr->base.end = &current_token;
            advance_token;
        } break;
        case TOK_OPEN_PAREN: {
            if (is_null_AST(n)) {
                // regular old paren expression
                n = new_ast_node_p(p, AST_paren_expr);
                n.as_paren_expr->base.start = &current_token;
                advance_token;

                n.as_paren_expr->subexpr = parse_expr(p, false);
                if (is_null_AST(n.as_paren_expr->subexpr))
                    error_at_parser(p, "expected expression");

                if (current_token.type != TOK_CLOSE_PAREN)
                    error_at_parser(p, "expected ')'");
                
                n.as_paren_expr->base.end = &current_token;
                advance_token;
            } else {
                // function call!
                AST temp;
                temp = new_ast_node_p(p, AST_call_expr);
                temp.as_call_expr->base.start = n.base->start;
                advance_token;

                da_init(&temp.as_call_expr->params, 4);
                while (current_token.type != TOK_CLOSE_PAREN) {
                    AST expr = parse_expr(p, false);
                    if (is_null_AST(expr)) {
                        error_at_parser(p, "expected expression");
                    }
                    da_append(&temp.as_call_expr->params, expr);
                    if (current_token.type == TOK_COMMA) {
                        advance_token;
                        continue;
                    } else if (current_token.type == TOK_CLOSE_PAREN) {
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
        } break;
        case TOK_PERIOD: {
            if (is_null_AST(n)) {
                // implicit selector
                n = new_ast_node_p(p, AST_impl_selector_expr);
                
                n.as_impl_selector_expr->base.start = &current_token;
                advance_token;
                
                if (current_token.type != TOK_IDENTIFIER)
                    error_at_parser(p, "expected identifer after '.'");
                
                AST temp = new_ast_node_p(p, AST_identifier_expr);

                temp.as_identifier_expr->base.end = &current_token;
                temp.as_identifier_expr->base.start = &current_token;
                temp.as_identifier_expr->tok = &current_token;

                n.as_impl_selector_expr->rhs = temp;

                n.as_impl_selector_expr->base.end = &current_token;
                advance_token;

            } else {
                // regular selector
                AST temp = new_ast_node_p(p, AST_selector_expr);
                temp.as_selector_expr->base.start = n.base->start;
                advance_token;

                if (current_token.type != TOK_IDENTIFIER)
                    error_at_parser(p, "expected identifer after '.'");

                AST ident = new_ast_node_p(p, AST_identifier_expr);
                
                ident.as_identifier_expr->base.end = &current_token;
                ident.as_identifier_expr->base.start = &current_token;
                ident.as_identifier_expr->tok = &current_token;
                temp.as_selector_expr->base.end = &current_token;

                temp.as_selector_expr->rhs = ident;
                temp.as_selector_expr->lhs = n;
                n = temp;
                advance_token;
            }
        } break;
        case TOK_COLON_COLON: {
            // entity selection
            if (is_null_AST(n)) {
                error_at_parser(p, "expected expression before '::'");
            } else {
                AST temp = new_ast_node_p(p, AST_entity_selector_expr);
                temp.as_entity_selector_expr->base.start = n.base->start;
                advance_token;

                if (current_token.type != TOK_IDENTIFIER)
                    error_at_parser(p, "expected identifer after '::'");

                AST ident = new_ast_node_p(p, AST_identifier_expr);
                
                ident.as_identifier_expr->base.end = &current_token;
                ident.as_identifier_expr->base.start = &current_token;
                ident.as_identifier_expr->tok = &current_token;
                temp.as_entity_selector_expr->base.end = &current_token;

                temp.as_entity_selector_expr->rhs = ident;
                temp.as_entity_selector_expr->lhs = n;
                n = temp;
                advance_token;
            }
        } break;
        case TOK_ARROW_RIGHT: {
            // return selection
            if (is_null_AST(n)) {
                error_at_parser(p, "expected expression before '->'");
            } else {
                AST temp = new_ast_node_p(p, AST_return_selector_expr);
                temp.as_return_selector_expr->base.start = n.base->start;
                advance_token;

                if (current_token.type != TOK_IDENTIFIER)
                    error_at_parser(p, "expected identifer after '->'");

                AST ident = new_ast_node_p(p, AST_identifier_expr);
                
                ident.as_identifier_expr->base.end = &current_token;
                ident.as_identifier_expr->base.start = &current_token;
                ident.as_identifier_expr->tok = &current_token;
                temp.as_return_selector_expr->base.end = &current_token;

                temp.as_entity_selector_expr->rhs = ident;
                temp.as_entity_selector_expr->lhs = n;
                n = temp;
                advance_token;
            }
        } break;
        case TOK_OPEN_BRACKET: {

            if (is_null_AST(n))
                error_at_parser(p, "expected expression before slice/index expr (THIS SHOULD NEVER HAPPEN - CONTACT SANDWICH)");

            advance_token;

            AST first_expr = parse_expr(p, false);
            if (current_token.type == TOK_CLOSE_BRACKET) {   // index expr
                if (is_null_AST(first_expr))
                    error_at_parser(p, "expected an expression inside '[]'");

                AST temp = new_ast_node_p(p, AST_index_expr);
                temp.as_index_expr->base.start = n.base->start;
                temp.as_index_expr->base.end = &current_token;
                temp.as_index_expr->lhs = n;
                temp.as_index_expr->inside = first_expr;
                n = temp;
                advance_token;
            } else if (current_token.type == TOK_COLON) {    // slice expr
                advance_token;
                AST temp = new_ast_node_p(p, AST_slice_expr);
                temp.as_slice_expr->base.start = n.base->start;
                temp.as_slice_expr->lhs = n;
                temp.as_slice_expr->inside_left = first_expr;

                temp.as_slice_expr->inside_right = parse_expr(p, false);
                if (current_token.type != TOK_CLOSE_BRACKET)
                    error_at_parser(p, "expected closing ']'");

                temp.as_slice_expr->base.end = &current_token;
                n = temp;
                advance_token;
            } else {
                error_at_parser(p, "expected ':' or ']'");
            }
        } break;
        case TOK_CARET: {
            if (is_null_AST(n))
                error_at_parser(p, "expected expression before deref (THIS SHOULD NEVER HAPPEN - CONTACT SANDWICH)");
            
            AST temp = new_ast_node_p(p, AST_unary_op_expr);
            temp.as_unary_op_expr->base.start = n.base->start;
            temp.as_unary_op_expr->base.end = &current_token;
            temp.as_unary_op_expr->op = &current_token;
            temp.as_unary_op_expr->inside = n;

            n = temp;
            advance_token;
        } break;
        case TOK_KEYWORD_STRUCT: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node_p(p, AST_struct_type_expr);

            n.base->start = &current_token;
            advance_token;

            while (current_token.type == TOK_HASH) {
                advance_token;
                if (current_token.type != TOK_IDENTIFIER)
                    error_at_parser(p, "expected an identifer");

                if (string_eq(current_token.text, str("smart_pack"))) {
                    n.as_struct_type_expr->smart_pack = true;
                    advance_token;
                } else {
                    error_at_parser(p, "unrecognized directive '"str_fmt"'", str_arg(current_token.text));
                }
            }

            if (current_token.type != TOK_OPEN_BRACE)
                error_at_parser(p, "expected '{' after 'struct'");
            advance_token;

            if (current_token.type == TOK_CLOSE_BRACE) {
                da_init(&n.as_struct_type_expr->fields, 1);
                advance_token;
                break;
            }

            da_init(&n.as_struct_type_expr->fields, 8);
            
            parse_typed_field_list(p, &n.as_struct_type_expr->fields, TOK_CLOSE_BRACE);
            n.base->end = &current_token;
            advance_token;
        } break;
        case TOK_KEYWORD_UNION: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node_p(p, AST_union_type_expr);

            n.base->start = &current_token;
            advance_token;

            if (current_token.type != TOK_OPEN_BRACE)
                error_at_parser(p, "expected '{' after 'struct'");
            advance_token;

            if (current_token.type == TOK_CLOSE_BRACE) {
                da_init(&n.as_union_type_expr->fields, 1);
                advance_token;
                break;
            }

            da_init(&n.as_union_type_expr->fields, 8);

            parse_typed_field_list(p, &n.as_union_type_expr->fields, TOK_CLOSE_BRACE);
            n.base->end = &current_token;
            advance_token;
        } break;
        case TOK_KEYWORD_ENUM: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node_p(p, AST_enum_type_expr);
            n.base->start = &current_token;
            advance_token;


            // parse optional backing type
            if (current_token.type != TOK_OPEN_BRACE) {
                AST enum_type = parse_type_expr(p);
                if (is_null_AST(enum_type)) {
                    error_at_parser(p, "expected a type");
                }
                n.as_enum_type_expr->backing_type = enum_type;
            }

            if (current_token.type != TOK_OPEN_BRACE)
                error_at_parser(p, "expected '{'");
            advance_token;



            if (current_token.type == TOK_CLOSE_BRACE) {
                da_init(&n.as_enum_type_expr->variants, 1);
                n.base->end = &current_token;
                advance_token;
                break;
            } else {
                da_init(&n.as_enum_type_expr->variants, 8); // 8 is probably an ok size
            }

            u64 value = 0;
            while (true) {

                AST variant = parse_expr(p, true);
                if (is_null_AST(variant))
                    error_at_parser(p, "expected field name");
                if (variant.type != AST_identifier_expr)
                    error_at_AST(p, variant, "enum variant must be an identifier");
                

                if (current_token.type == TOK_EQUAL) {
                    advance_token;
                    if (current_token.type != TOK_LITERAL_INT)
                        error_at_parser(p, "enum variant value must be an integer literal");

                    value = int_lit_value(p);
                    advance_token;
                }

                da_append(&n.as_enum_type_expr->variants, ((AST_enum_variant){
                    variant,
                    value
                }));
                
                value++;

                if (current_token.type == TOK_COMMA) {
                    advance_token;
                    if (current_token.type == TOK_CLOSE_BRACE) {
                        break;
                    } else {
                        continue;
                    }
                } else if (current_token.type == TOK_CLOSE_BRACE) {
                    break;
                } else {
                    error_at_parser(p, "expected ',' or '}'");
                }

            }

            n.base->end = &current_token;
            advance_token;
        } break;
        case TOK_KEYWORD_FN: {
            if (!is_null_AST(n)) {
                out = true;
                break;
            }
            n = new_ast_node_p(p, AST_fn_type_expr);
            n.base->start = &current_token;
            advance_token;

            while (current_token.type == TOK_HASH) {
                // parse a secondary # tag
                advance_token;
                if (current_token.type != TOK_IDENTIFIER)
                    error_at_parser(p, "expected an identifer");

                if (string_eq(current_token.text, str("block_symbol"))) {
                    advance_token;
                    if (current_token.type != TOK_OPEN_PAREN)
                        error_at_parser(p, "expected '('");
                    advance_token;

                    n.as_fn_type_expr->block_symbol_override = parse_expr(p, false);

                    if (n.as_fn_type_expr->block_symbol_override.type != AST_literal_expr)
                        error_at_AST(p,n.as_fn_type_expr->block_symbol_override, "expected string literal");

                    if (current_token.type != TOK_CLOSE_PAREN)
                        error_at_parser(p, "expected ')'");
                    advance_token;
                } else if (string_eq(current_token.text, str("always_inline"))) {
                    advance_token;
                    n.as_fn_type_expr->always_inline = true;
                }
            }



            if (current_token.type != TOK_OPEN_PAREN)
                error_at_parser(p, "expected '(' after 'fn'");
            advance_token;


            da_init(&n.as_fn_type_expr->parameters, 4);
            da_init(&n.as_fn_type_expr->returns, 2);

            parse_typed_field_list(p, &n.as_fn_type_expr->parameters, TOK_CLOSE_PAREN);

            n.base->end = &current_token;
            advance_token;

            // no return type
            if (current_token.type != TOK_ARROW_RIGHT) {
                break;
            }
            advance_token;

            // simple return type (single, untyped value)
            if (current_token.type != TOK_OPEN_PAREN) {
                AST return_type = parse_type_expr(p);
                if (is_null_AST(return_type))
                    error_at_parser(p, "expected a type expression or list of returns");
                if (!is_possible_type_expr(return_type))
                    error_at_AST(p, return_type, "return type must be a type expression");

                n.as_fn_type_expr->simple_return = true;
                da_append(&n.as_fn_type_expr->returns, ((AST_typed_field){
                    NULL_AST, return_type
                }));
                n.base->end = &peek_token(-1);
                break;
            }

            // else, complex return
            advance_token;
            parse_typed_field_list(p, &n.as_fn_type_expr->returns, TOK_CLOSE_PAREN);
            n.base->end = &current_token;
            advance_token;

        } break;
        case TOK_OPEN_BRACE: {

            if (no_cl) {
                out = true;
                break;
            }

            // if previously parsed expression exists and isnt a type, return that expression
            if (!is_null_AST(n) && is_definitely_not_type_expr_trust_me_bro(n)) {
                out = true;
                break;
            }

            if (n.type == AST_fn_type_expr) {
                AST func_lit = new_ast_node_p(p, AST_func_literal_expr);
                func_lit.as_func_literal_expr->base.start = n.base->start;
                func_lit.as_func_literal_expr->type = n;
                
                AST code_block = parse_block_stmt(p);
                func_lit.as_func_literal_expr->code_block = code_block;

                func_lit.as_func_literal_expr->base.end = code_block.base->end;
                n = func_lit;
                break;
            }

            AST lit = new_ast_node_p(p, AST_comp_literal_expr);
            lit.as_comp_literal_expr->type = n;

            if (!is_null_AST(n)) {
                lit.base->start = n.base->start;
            } else {
                lit.base->start = &current_token;
            }

            advance_token;
            if (current_token.type == TOK_CLOSE_BRACE) {
                da_init(&lit.as_comp_literal_expr->elems, 1); // we might not need to do this
                lit.base->end = &current_token;
                n = lit;
                advance_token;
                break;
            }

            da_init(&lit.as_comp_literal_expr->elems, 4);

            while (true) {
                AST expr = parse_expr(p, false);

                if (is_null_AST(expr))
                    error_at_parser(p, "expected an expression");

                da_append(&lit.as_comp_literal_expr->elems, expr);

                if (current_token.type == TOK_COMMA) {
                    advance_token;
                    if (current_token.type == TOK_CLOSE_BRACE) {
                        break;
                    } else {
                        continue;
                    }
                } else if (current_token.type == TOK_CLOSE_BRACE) {
                    break;
                } else {
                    error_at_parser(p, "expected ',' or '}'");
                }
            }

            // TODO("enum shittery");
            lit.base->end = &current_token;
            n = lit;
            advance_token;
        } break;
        default:
            out = true;
            break;
        }
    }
    return n;
}

AST parse_import_stmt(parser* p) {
    AST n = new_ast_node_p(p, AST_import_stmt);
    n.as_import_stmt->base.start = &current_token;
    advance_token;
    
    if (current_token.type != TOK_IDENTIFIER)
        error_at_parser(p, "expected identifier for module import");

    n.as_import_stmt->name = parse_expr(p, true);

    if (n.as_import_stmt->name.type != AST_identifier_expr)
        error_at_AST(p, n.as_import_stmt->name, "expected an identifer");

    if (current_token.type != TOK_LITERAL_STRING)
        error_at_parser(p, "expected import path");
    
    n.as_import_stmt->path = parse_expr(p, true);
    
    n.as_import_stmt->base.end = &current_token;

    if (current_token.type != TOK_SEMICOLON)
        error_at_parser(p, "expected ';'");
    
    advance_token;
    return n;
}

AST parse_stmt(parser* p) {
    AST n = NULL_AST;

    switch (current_token.type) {
    case TOK_KEYWORD_IMPORT: {
        n = parse_import_stmt(p);
    } break;
    case TOK_KEYWORD_EXTERN: {
        n = new_ast_node_p(p, AST_extern_stmt);
        n.as_extern_stmt->base.start = &current_token;
        advance_token;

        n.as_extern_stmt->decl = parse_stmt(p);

        n.as_extern_stmt->base.end = &peek_token(-1);
    } break;
    case TOK_KEYWORD_DEFER: {
        n = new_ast_node_p(p, AST_defer_stmt);
        n.as_defer_stmt->base.start = &current_token;
        advance_token;

        n.as_defer_stmt->stmt = parse_stmt(p);

        n.as_defer_stmt->base.end = &peek_token(-1);
    } break;
    case TOK_KEYWORD_IF: {
        n = new_ast_node_p(p, AST_if_stmt);
        n.as_if_stmt->is_elif = false;
        n.as_if_stmt->base.start = &current_token;
        advance_token;

        n.as_if_stmt->condition = parse_expr(p, true);
        n.as_if_stmt->if_branch = parse_block_stmt(p);
        
        n.as_if_stmt->base.end = &peek_token(-1);

        if (current_token.type == TOK_KEYWORD_ELIF) {
            n.as_if_stmt->else_branch = parse_elif(p);
        }
        else if (current_token.type == TOK_KEYWORD_ELSE) {
            advance_token;
            n.as_if_stmt->else_branch = parse_block_stmt(p);
        }
    } break;
    case TOK_KEYWORD_WHILE: {
        n = new_ast_node_p(p, AST_while_stmt);
        n.as_while_stmt->base.start = &current_token;
        advance_token;

        n.as_while_stmt->condition = parse_expr(p, true);
        if (n.as_while_stmt->condition.rawptr == NULL) {

        }
        n.as_while_stmt->block = parse_block_stmt(p);

        n.as_while_stmt->base.end = &peek_token(-1);
    } break;
    case TOK_KEYWORD_FOR: {
        // jank as shit but whatever, that's future sandwichman's problem
        if ((peek_token(1).type == TOK_IDENTIFIER || peek_token(1).type == TOK_IDENTIFIER_DISCARD) &&
        (peek_token(2).type == TOK_COLON || peek_token(2).type == TOK_KEYWORD_IN)) {
            // for-in loop!
            n = new_ast_node_p(p, AST_for_in_stmt);
            n.as_for_in_stmt->base.start = &current_token;
            advance_token;

            n.as_for_in_stmt->indexvar = parse_expr(p, true);

            if (current_token.type == TOK_COLON) {
                advance_token;
                n.as_for_in_stmt->type = parse_expr(p, true);
                if (is_null_AST(n.as_for_in_stmt->type))
                    error_at_parser(p, "expected type expression after ':'");
            }

            if (current_token.type != TOK_KEYWORD_IN)
                error_at_parser(p, "expected 'in'");
            advance_token;

            // catch #reverse
            if (current_token.type == TOK_HASH) {
                advance_token;
                if (string_eq(current_token.text, str("reverse"))) {
                    n.as_for_in_stmt->is_reverse = true;
                    advance_token;
                } else {
                    error_at_parser(p, "invalid directive \'"str_fmt"\' in for-in loop", current_token.text);
                }
            }


            n.as_for_in_stmt->from = parse_expr(p, true);
            if (current_token.type == TOK_RANGE_LESS) {
                n.as_for_in_stmt->is_inclusive = false;
            } else if (current_token.type == TOK_RANGE_EQ) {
                n.as_for_in_stmt->is_inclusive = true;
            } else {
                error_at_parser(p, "expected '..<' or '..='");
            }
            advance_token;
            
            n.as_for_in_stmt->to = parse_expr(p, true);
            n.as_for_in_stmt->block = parse_block_stmt(p);
            
            n.as_for_in_stmt->base.end = &peek_token(-1);
        } else {
            // regular for loop!
            n = new_ast_node_p(p, AST_for_stmt);
            n.as_for_stmt->base.start = &current_token;
            advance_token;

            n.as_for_stmt->prelude = parse_stmt(p);
            
            n.as_for_stmt->condition = parse_expr(p, true);
            if (is_null_AST(n.as_for_stmt->condition))
                error_at_parser(p, "expected an expression");

            if (current_token.type != TOK_SEMICOLON)
                error_at_parser(p, "expected ';'");

            advance_token;
            n.as_for_stmt->update = parse_stmt(p);
            
            n.as_for_stmt->block = parse_block_stmt(p);

            n.as_for_stmt->base.end = &peek_token(-1);
        }
    } break;
    case TOK_KEYWORD_SWITCH: {
       
       
        n = new_ast_node_p(p, AST_switch_stmt);
        n.base->start = &current_token;
        advance_token;

        n.as_switch_stmt->expr = parse_expr(p, true);
        if (is_null_AST(n))
            error_at_parser(p, "expected an expression");

        if (current_token.type != TOK_KEYWORD_CASE)
            error_at_parser(p, "expected 'case'");

        while (current_token.type == TOK_KEYWORD_CASE) {
            AST case_stmt = new_ast_node_p(p, AST_case);
            case_stmt.base->start = &current_token;
            advance_token;

            da_init(&case_stmt.as_case->matches, 1);
            while (current_token.type != TOK_OPEN_BRACE) {
                AST expr = parse_expr(p, false);
                if (is_null_AST(expr)) {
                    error_at_parser(p, "expected expression");
                }
                da_append(&case_stmt.as_case->matches, expr);
                if (current_token.type == TOK_COMMA) {
                    advance_token;
                    continue;
                } else if (current_token.type == TOK_OPEN_BRACE) {
                    break;
                } else {
                    error_at_parser(p, "expected ',' or '{'");
                }
            }

            n.as_case->block = parse_block_stmt(p);
            case_stmt.base->end = &peek_token(-1);
        }

        n.base->end = &peek_token(-1);
        // error_at_parser(p, "TODO switch statement");

    } break;
    case TOK_SEMICOLON: {
        n = new_ast_node_p(p, AST_empty_stmt);
        n.as_empty_stmt->base.start = &current_token;
        n.as_empty_stmt->base.end = &current_token;
        advance_token;
    } break;
    case TOK_OPEN_BRACKET: {
        n = parse_block_stmt(p);
    } break;
    case TOK_KEYWORD_TYPE: {
        n = new_ast_node_p(p, AST_type_decl_stmt);
        n.as_type_decl_stmt->base.start = &current_token;

        advance_token;
        if (current_token.type != TOK_IDENTIFIER)
            error_at_parser(p, "expected an identifier");

        n.as_type_decl_stmt->lhs = parse_expr(p, true);
        
        if (current_token.type != TOK_EQUAL)
            error_at_parser(p, "expected '='");
        advance_token;

        n.as_type_decl_stmt->rhs = parse_expr(p, true);
        if (is_null_AST(n.as_type_decl_stmt->rhs))
            error_at_parser(p, "expected type expression");

        if (current_token.type != TOK_SEMICOLON)
            error_at_parser(p, "expected ';'");
        
        n.as_type_decl_stmt->base.end = &current_token;
        advance_token;
    } break;
    case TOK_KEYWORD_LET:
    case TOK_KEYWORD_MUT: {
        n = new_ast_node_p(p, AST_decl_stmt);
        n.as_decl_stmt->base.start = &current_token;
        n.as_decl_stmt->is_mut = (current_token.type == TOK_KEYWORD_MUT);
        advance_token;

        // maybe directives?
        if (current_token.type == TOK_HASH) {
            advance_token;
            if (string_eq(current_token.text, str("static"))) {
                n.as_decl_stmt->is_static = true;
            } else if (string_eq(current_token.text, str("volatile"))) {
                n.as_decl_stmt->is_volatile = true;
            } else {
                error_at_parser(p, "invalid directive \'%s\' in declaration", clone_to_cstring(current_token.text));
            }
        }

        // parse identifier list
        da_init(&n.as_decl_stmt->lhs, 4); // four is probably good right
        while (current_token.type != TOK_IDENTIFIER || current_token.type != TOK_IDENTIFIER_DISCARD) {
            da_append(&n.as_decl_stmt->lhs, parse_expr(p, true));
            if (current_token.type != TOK_COMMA)
                break;
            advance_token;
        }

        // maybe types?
        if (current_token.type == TOK_COLON) {
            n.as_decl_stmt->has_expl_type = true;
            advance_token;
            n.as_decl_stmt->type = parse_expr(p, true);
            if (is_null_AST(n.as_decl_stmt->type))
                error_at_parser(p, "expected type expression");
        }

        // implicit initialization ONLY if a type has been provided
        if (current_token.type == TOK_SEMICOLON) {
            n.as_decl_stmt->base.end = &current_token;
            advance_token;
            break;
        }

        if (current_token.type != TOK_EQUAL)
            error_at_parser(p, "expected '='");
        
        advance_token;

        if (current_token.type == TOK_UNINIT) {
            n.as_decl_stmt->is_uninit = true;
            advance_token;
        } else {
            n.as_decl_stmt->rhs = parse_expr(p, false);
        }
        
        if (current_token.type != TOK_SEMICOLON)
            error_at_parser(p, "expected ';'");

        n.as_decl_stmt->base.end = &current_token;
        advance_token;
    } break;
    case TOK_KEYWORD_BREAK: {
        n = new_ast_node_p(p, AST_break_stmt);
        n.base->start = &current_token;
        advance_token;
        if (current_token.type != TOK_SEMICOLON) {
            n.as_break_stmt->label = parse_expr(p, false);
            if (is_null_AST(n.as_break_stmt->label) || n.as_break_stmt->label.type != AST_identifier_expr)
                error_at_token(p, *n.base->start, "expected an identifer");
        }

        if (current_token.type != TOK_SEMICOLON)
            error_at_parser(p, "expected ';'");

        n.base->end = &current_token;
        advance_token;
    } break;
    case TOK_KEYWORD_CONTINUE: {
        n = new_ast_node_p(p, AST_continue_stmt);
        n.base->start = &current_token;
        advance_token;
        if (current_token.type != TOK_SEMICOLON) {
            n.as_continue_stmt->label = parse_expr(p, false);
            if (is_null_AST(n.as_continue_stmt->label) || n.as_continue_stmt->label.type != AST_identifier_expr)
                error_at_token(p, *n.base->start, "expected an identifer");
        }

        if (current_token.type != TOK_SEMICOLON)
            error_at_parser(p, "expected ';'");

        n.base->end = &current_token;
        advance_token;
    } break;
    // case TOK_KEYWORD_GOTO: {
    //     n = new_ast_node_p(p, AST_goto_stmt);
    //     n.base->start = &current_token;
    //     advance_token;
    //     if (current_token.type != TOK_SEMICOLON) {
    //         n.as_goto_stmt->label = parse_expr(p, false);
    //         if (is_null_AST(n.as_goto_stmt->label) || n.as_goto_stmt->label.type != AST_identifier_expr)
    //             error_at_token(p, *n.base->start, "expected an identifer");
    //     }

    //     if (current_token.type != TOK_SEMICOLON)
    //         error_at_parser(p, "expected ';'");

    //     n.base->end = &current_token;
    //     advance_token;
    // } break;
    case TOK_KEYWORD_FALLTHROUGH: {
        n = new_ast_node_p(p, AST_fallthrough_stmt);
        n.base->start = &current_token;
        advance_token;
        if (current_token.type != TOK_SEMICOLON)
            error_at_parser(p, "expected ';'");
        n.base->end = &current_token;
        advance_token;
    } break;
    case TOK_KEYWORD_RETURN: {
        n = new_ast_node_p(p, AST_return_stmt);
        n.base->start = &current_token;
        advance_token;

        da_init(&n.as_return_stmt->returns, 2);

        while (current_token.type != TOK_SEMICOLON) {
            AST ret_expr = parse_expr(p, false);
            if (is_null_AST(ret_expr))
                error_at_parser(p, "expected expression");
            da_append(&n.as_return_stmt->returns, ret_expr);
            if (current_token.type == TOK_COMMA)
                advance_token;
            else if (current_token.type == TOK_SEMICOLON)
                break;
            else
                error_at_parser(p, "expected ',' or ';'");
        }
        
        n.base->end = &current_token;
        advance_token;
    } break;
    case TOK_KEYWORD_ASM: {
        error_at_parser(p, "TODO inline assembly");
    } break;
    case TOK_KEYWORD_CASE: error_at_parser(p, "'case' only allowed with a switch statement");
    default: {
        AST lhs = parse_expr(p, true);
        
        if (is_null_AST(lhs)) return lhs;

        if (current_token.type == TOK_SEMICOLON) {
            n = new_ast_node_p(p, AST_expr_stmt);

            n.base->start = lhs.base->start;
            n.base->end = &current_token;

            n.as_expr_stmt->expression = lhs;

            advance_token;
            break;
        }

        switch (current_token.type) {
        case TOK_EQUAL: {    // single-assign statement
            n = new_ast_node_p(p, AST_assign_stmt);
            n.as_assign_stmt->base.start = lhs.base->start;
            da_init(&n.as_assign_stmt->lhs, 1);
            da_append(&n.as_assign_stmt->lhs, lhs);

            advance_token;

            n.as_assign_stmt->rhs = parse_expr(p, false);
            if (is_null_AST(n.as_assign_stmt->rhs))
                error_at_parser(p, "expected an expression");
            
            if (current_token.type != TOK_SEMICOLON)
                error_at_parser(p, "expected ';'");

            n.as_assign_stmt->base.end = &current_token;
            advance_token;
        } break;
        case TOK_COMMA: {    // multi-assign statement
            n = new_ast_node_p(p, AST_assign_stmt);
            n.as_assign_stmt->base.start = n.base->start;
            da_init(&n.as_assign_stmt->lhs, 2);
            da_append(&n.as_assign_stmt->lhs, lhs);
            advance_token;

            while (true) {
                AST expr = parse_expr(p, false);

                if (is_null_AST(expr))
                    error_at_parser(p, "expected an expression");

                da_append(&n.as_assign_stmt->lhs, expr);

                if (current_token.type == TOK_COMMA) {
                    advance_token;
                    if (current_token.type == TOK_EQUAL) {
                        break;
                    } else {
                        continue;
                    }
                } else if (current_token.type == TOK_EQUAL) {
                    break;
                } else {
                    error_at_parser(p, "expected ',' or '='");
                }
            }
            advance_token;

            n.as_assign_stmt->rhs = parse_expr(p, false);
            if (is_null_AST(n.as_assign_stmt->rhs))
                error_at_parser(p, "expected an expression");
            
            if (current_token.type != TOK_SEMICOLON)
                error_at_parser(p, "expected ';'");

            n.as_assign_stmt->base.end = &current_token;
            advance_token;
        } break;
        case TOK_COLON: {    // statement label
            n = new_ast_node_p(p, AST_label_stmt);
            n.as_label_stmt->base.start = lhs.base->start;
            n.as_label_stmt->base.end = &current_token;
            n.as_label_stmt->label = lhs;
            advance_token;
        } break;
        case TOK_ADD_EQUAL: // compound assign statement
        case TOK_SUB_EQUAL:
        case TOK_MUL_EQUAL:
        case TOK_DIV_EQUAL:
        case TOK_MOD_EQUAL:
        case TOK_MOD_MOD_EQUAL:
        case TOK_AND_EQUAL:
        case TOK_OR_EQUAL:
        case TOK_NOR_EQUAL:
        case TOK_XOR_EQUAL:
        case TOK_LSHIFT_EQUAL:
        case TOK_RSHIFT_EQUAL: {
            n = new_ast_node_p(p, AST_comp_assign_stmt);
            n.as_comp_assign_stmt->base.start = lhs.base->start;
            n.as_comp_assign_stmt->lhs = lhs;

            n.as_comp_assign_stmt->op = &current_token;
            advance_token;

            n.as_comp_assign_stmt->rhs = parse_expr(p, false);
            if (is_null_AST(n.as_comp_assign_stmt->rhs))
                error_at_parser(p, "expected an expression");
            
            if (current_token.type != TOK_SEMICOLON)
                error_at_parser(p, "expected ';'");

            n.as_comp_assign_stmt->base.end = &current_token;
            advance_token;
        } break;
        default:
            error_at_parser(p, "expected ';'");
        }
    } break;
    }
    return n;
}

// only for use within parse_stmt
AST parse_elif(parser* p) {
    AST n = new_ast_node_p(p, AST_if_stmt);
    n.as_if_stmt->is_elif = true;
    n.as_if_stmt->base.start = &current_token;

    advance_token;

    // get condition expression
    n.as_if_stmt->condition = parse_expr(p, true);
        
    // get block
    n.as_if_stmt->if_branch = parse_block_stmt(p);

    n.as_if_stmt->base.end = &peek_token(-1);

    if (current_token.type == TOK_KEYWORD_ELIF) {
        n.as_if_stmt->else_branch = parse_elif(p);
    }
    else if (current_token.type == TOK_KEYWORD_ELSE) {
        advance_token;
        n.as_if_stmt->else_branch = parse_block_stmt(p);
    }
    return n;
}

AST parse_block_stmt(parser* p) {
    AST n = new_ast_node_p(p, AST_block_stmt);

    if (current_token.type != TOK_OPEN_BRACE)
        error_at_parser(p, "expected '{'");
    n.as_block_stmt->base.start = &current_token;

    da_init(&n.as_block_stmt->stmts, 1);

    advance_token;
    while (current_token.type != TOK_CLOSE_BRACE) {
        AST stmt = parse_stmt(p);
        da_append(&n.as_block_stmt->stmts, stmt);
    }

    if (current_token.type != TOK_CLOSE_BRACE)
        error_at_parser(p, "expected '}'");
    n.as_block_stmt->base.end = &current_token;
    advance_token;
    return n;
}