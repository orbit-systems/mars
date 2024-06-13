#include "orbit.h"
#include "term.h"

#include "parse.h"

// kayla's FREEZINGLY 🧊🧊 SLOW 🐌🐌 parser in ALGOL69 👴👴 + KITTY POWERED with KIBBLE 5.0 🐈🐈 and DOGECOIN WEB4 TECHNOLOGY

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

void parse_file(parser* p) {
    //<program> ::= <module_stmt> <import_stmt>* <stmts>
    p->module_decl = parse_module_decl(p);

    da_init(&p->stmts, 1);

    while (current_token(p).type == TOK_KEYWORD_IMPORT) da_append(&p->stmts, parse_import_decl(p));

    while (current_token(p).type != TOK_EOF) {
        da_append(&p->stmts, parse_stmt(p));
        
        //general_error("Encountered unexpected token type %s", token_type_str[current_token(p).type]);
    }
}

AST parse_stmt(parser* p) {
    /*<stmt> ::= <decl> | <return> | <if> | <while> | <assignment> | 
           <aggregate> | <enum> | <for_type_one> | <for_type_two> | <asm> | <stmt_block> |
           <switch> | <break> | <fallthrough> | <expression> ";" | <extern> | <defer> | <continue> | ";"*/
    AST n;
    switch (current_token(p).type) {
        //<decl>
        case TOK_KEYWORD_LET:
        case TOK_KEYWORD_MUT:
            return parse_decl_stmt(p);
        //<return> ::= "return" (<expression>* ",") <expression> ";"
        case TOK_KEYWORD_RETURN:
            n = new_ast_node(p, AST_return_stmt);
            n.as_return_stmt->base.start = &current_token(p);
            advance_token(p);
            da_init(&n.as_return_stmt->returns, 1);
            while (current_token(p).type != TOK_SEMICOLON) {
                da_append(&n.as_return_stmt->returns, parse_expr(p));
                advance_token(p);
            }
            n.as_return_stmt->base.end = &current_token(p);
            return n;
        /**/
        case TOK_SEMICOLON:
            n = new_ast_node(p, AST_empty_stmt);
            n.as_empty_stmt->tok = &current_token(p);
            n.as_empty_stmt->base.start = &current_token(p);
            n.as_empty_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
    }
    return NULL_AST;
}

// kayla what? a statement block is only { <stmt?* }
// AST parse_stmt_block(parser* p) {
//     //<stmt_block> ::=  "{" <stmt>* "}" | <stmt>
//     AST n = new_ast_node(p, AST_stmt_block); 
//     da_init(&n.as_stmt_block->stmts, 1);

//     if (current_token(p).type != TOK_OPEN_BRACE) {
//         AST stmt = parse_stmt(p);
//         if (is_null_AST(stmt)) error_at_parser(p, "expected statement");
//         da_append(&n.as_stmt_block->stmts, stmt);
//         return n;
//     }

//     advance_token(p);
//     while (current_token(p).type != TOK_CLOSE_BRACE) {
//         AST stmt = parse_stmt(p);
//         if (is_null_AST(stmt)) error_at_parser(p, "expected statement");
//         da_append(&n.as_stmt_block->stmts, stmt);
//     }
//     advance_token(p);
//     return n;
// }

AST parse_stmt_block(parser* p) {
    //<stmt_block> ::=  "{" <stmt>* "}" | <stmt>
    AST n = new_ast_node(p, AST_stmt_block); 
    da_init(&n.as_stmt_block->stmts, 2);

    if (current_token(p).type != TOK_OPEN_BRACE) {
        error_at_parser(p, "expected {");
    }
    advance_token(p);

    while (current_token(p).type != TOK_CLOSE_BRACE) {
        AST stmt = parse_stmt(p);
        if (is_null_AST(stmt)) break;
        da_append(&n.as_stmt_block->stmts, stmt);
    }
    advance_token(p);
    return n;
}

static AST parse_identifier(parser* p) {
    AST i = new_ast_node(p, AST_identifier);
    i.as_identifier->tok = i.base->start = i.base->end = &current_token(p);
    advance_token(p);
    return i;
}

AST parse_atomic_expr(parser* p) {
    AST left = NULL_AST;
    AST old_left = NULL_AST;

    while (true) switch (current_token(p).type) {
    case TOK_OPEN_PAREN:
        if (is_null_AST(left)) {
            // <paren_expr> ::= "(" <expr> ")"
            advance_token(p);
            left = parse_expr(p);
            if (current_token(p).type != TOK_CLOSE_PAREN) {
                error_at_parser(p, "expected ')'");
            }
        } else {
            TODO("call expressions");
        }

        continue;
    case TOK_IDENTIFIER:
    case TOK_UNDERSCORE:
        if (!is_null_AST(left)) return left; // if we've already parsed smth, let it be

        left = parse_identifier(p);
        left.as_identifier->is_discard = left.base->start->type == TOK_UNDERSCORE;
        continue;

    case TOK_PERIOD:
        // atomic.field | .field

        old_left = left;
        left = new_ast_node(p, AST_selector_expr);
        left.as_selector_expr->lhs = left;
        left.as_selector_expr->op = &current_token(p);
        left.base->start = old_left.base->start;

        advance_token(p);

        if (current_token(p).type != TOK_IDENTIFIER) {
            error_at_parser(p, "expected identifier after selector");
        }
        left.base->end = &current_token(p);
        left.as_selector_expr->rhs = parse_identifier(p);
        continue;
    
    case TOK_ARROW_RIGHT:
        if (!is_null_AST(left)) error_at_parser(p, "left side of -> must not be empty");
    case TOK_COLON_COLON:
        if (!is_null_AST(left)) error_at_parser(p, "left side of :: must not be empty");

        old_left = left;
        left = new_ast_node(p, AST_selector_expr);
        left.as_selector_expr->lhs = left;
        left.as_selector_expr->op = &current_token(p);
        left.base->start = old_left.base->start;

        advance_token(p);

        if (current_token(p).type != TOK_IDENTIFIER) {
            error_at_parser(p, "expected identifier after selector");
        }
        left.base->end = &current_token(p);
        left.as_selector_expr->rhs = parse_identifier(p);
        continue;

    case TOK_OPEN_BRACE:
        if (!is_null_AST(left) && left.type == AST_fn_type_expr) {
            // we're in a function literal!
            old_left = left;
            left = new_ast_node(p, AST_func_literal_expr);
            left.base->start = old_left.base->start;
            left.as_func_literal_expr->type = old_left;
            left.as_func_literal_expr->code_block = parse_stmt_block(p);
            left.base->end = &peek_token(p, -1);
        } else {
            error_at_AST(p, left, "bruh");
            //compound literal!
            printf("\n!! %d \n", left.type);
            TODO("compound literal");
        }

        continue;
    case TOK_KEYWORD_FN:
        if (!is_null_AST(left)) return left;
        left = parse_fn_type(p);
        continue;

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
    case TOK_TYPE_KEYWORD_F64:
        if (!is_null_AST(left)) return left;

        left = new_ast_node(p, AST_basic_type_expr);
        left.as_basic_type_expr->lit = left.base->start = left.base->end = &current_token(p);
        advance_token(p);
        continue;
    default:
        return left;
    }

    return left;
}



AST parse_aggregate(parser* p) {
    //<aggregate> ::= ("struct" | "union")  "{" <param_list> "}" ";" 
    AST n = new_ast_node(p, AST_struct_type_expr);
    n.as_struct_type_expr->base.start = &current_token(p);
    n.as_struct_type_expr->is_union = current_token(p).type == TOK_KEYWORD_UNION;
    da_init(&n.as_struct_type_expr->fields, 1);
    advance_token(p);
    if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");
    advance_token(p);
    if (current_token(p).type != TOK_CLOSE_PAREN) { 
        /*        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>

        */
        for (int i = p->current_tok; i < p->tokens.len; i++) {
            if (current_token(p).type == TOK_COLON) error_at_parser(p, "unexpected :");
            if (current_token(p).type == TOK_COMMA) advance_token(p);
            if (current_token(p).type == TOK_CLOSE_PAREN) break;
            if (current_token(p).type == TOK_IDENTIFIER) {
                if (peek_token(p, 1).type == TOK_COLON) {
                    //<param> ::= <unary_expression> ":" <unary_expression>
                    AST_typed_field curr_field = {0};
                    AST field = parse_unary_expr(p);
                    advance_token(p);
                    da_append(&n.as_struct_type_expr->fields, ((AST_typed_field){.field = field, .type = parse_unary_expr(p)}));
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA) { 
                    //<multi_param> ::= (<unary_expression>  ",")+ <unary_expression> ":" <unary_expression> 
                    AST_typed_field curr_field;
                    curr_field.field = parse_unary_expr(p);
                    curr_field.type.type = 0;
                    da_append(&n.as_struct_type_expr->fields, curr_field);
                    advance_token(p);
                    continue;
                }
            }
            error_at_parser(p, "unexpected");
        }
    }
    advance_token(p);
    if (current_token(p).type == TOK_SEMICOLON) error_at_parser(p, "expected ;");
    n.as_struct_type_expr->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_enum(parser* p) {
    //enum's param_list separator is =, not :
    //<enum> ::= "enum" (<unary_expression> | E) "{" <param_list> "}" ";" 
    AST n = new_ast_node(p, AST_enum_type_expr);
    n.as_enum_type_expr->base.start = &current_token(p);
    n.as_enum_type_expr->backing_type = parse_unary_expr(p);
    da_init(&n.as_enum_type_expr->variants, 1);
    advance_token(p);
    if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");
    advance_token(p);
    if (current_token(p).type != TOK_CLOSE_PAREN) { 
        /*        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>    */
        for (int i = p->current_tok; i < p->tokens.len; i++) {
            if (current_token(p).type == TOK_EQUAL) error_at_parser(p, "unexpected :");
            if (current_token(p).type == TOK_COMMA) advance_token(p);
            if (current_token(p).type == TOK_CLOSE_PAREN) break;
            if (current_token(p).type == TOK_IDENTIFIER) {
                if (peek_token(p, 1).type == TOK_EQUAL) {
                    //<param> ::= <unary_expression> "=" <unary_expression>
                    AST_enum_variant var = {0};
                    var.ident = parse_unary_expr(p);
                    advance_token(p); advance_token(p);
                    var.tok = &current_token(p);
                    da_append(&n.as_enum_type_expr->variants, var);
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA) { 
                    //<multi_param> ::= (<unary_expression>  ",")+ <unary_expression> "=" <unary_expression> 
                    AST_enum_variant var = {0};
                    var.ident = parse_unary_expr(p);
                    advance_token(p); advance_token(p);
                    var.tok = NULL;
                    da_append(&n.as_enum_type_expr->variants, var);
                    continue;
                }
            }
            error_at_parser(p, "unexpected");
        }
    }
    advance_token(p);
    if (current_token(p).type == TOK_SEMICOLON) error_at_parser(p, "expected ;");
    n.as_enum_type_expr->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_fn_type(parser* p) {
    /*<fn_type> ::= "fn" "(" (<param_list> | E) ")" ("->" (<unary_expression> | "(" <param_list> ")") | E)

        <param> ::= <unary_expression> ":" <unary_expression> 
        <multi_param> ::= (<unary_expression>  ",")+ <unary_expression> ":" <unary_expression> 
        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>
        */

    AST n = new_ast_node(p, AST_fn_type_expr);
    n.as_fn_type_expr->base.start = &current_token(p);
    da_init(&n.as_fn_type_expr->parameters, 1);
    advance_token(p);
    if (current_token(p).type != TOK_OPEN_PAREN) error_at_parser(p, "expected (");
    advance_token(p);

    if (current_token(p).type != TOK_CLOSE_PAREN) { 
        /*        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>

        */
        for (int i = p->current_tok; i < p->tokens.len; i++) {
            if (current_token(p).type == TOK_COLON) error_at_parser(p, "unexpected :");
            if (current_token(p).type == TOK_COMMA) advance_token(p);
            if (current_token(p).type == TOK_CLOSE_PAREN) break;
            if (current_token(p).type == TOK_IDENTIFIER) {
                if (peek_token(p, 1).type == TOK_COLON) {
                    //<param> ::= <unary_expression> ":" <unary_expression>
                    AST_typed_field curr_field = {0};
                    AST field = parse_unary_expr(p);
                    advance_token(p);
                    da_append(&n.as_fn_type_expr->parameters, ((AST_typed_field){.field = field, .type = parse_unary_expr(p)}));
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA) { 
                    //<multi_param> ::= (<unary_expression>  ",")+ <unary_expression> ":" <unary_expression> 
                    AST_typed_field curr_field;
                    curr_field.field = parse_unary_expr(p);
                    curr_field.type.type = 0;
                    da_append(&n.as_fn_type_expr->parameters, curr_field);
                    advance_token(p);
                    continue;
                }
            }
            error_at_parser(p, "unexpected");
        }
    }
    if (current_token(p).type != TOK_CLOSE_PAREN) error_at_parser(p, "expected! )");
    advance_token(p);

    da_init(&n.as_fn_type_expr->returns, 1);
    if (current_token(p).type == TOK_ARROW_RIGHT) {
        //returns time!
        advance_token(p);
        if (current_token(p).type == TOK_OPEN_PAREN) {
            advance_token(p);
            //param parsing
            /*        <param_list> ::= <param_list> "," <param> | 
                             <param_list> "," <multi_param> | 
                             <param> | <multi_param>

            */
            for (int i = p->current_tok; i < p->tokens.len; i++) {
                if (current_token(p).type == TOK_COLON) error_at_parser(p, "unexpected :");
                if (current_token(p).type == TOK_COMMA) advance_token(p);
                if (current_token(p).type == TOK_CLOSE_PAREN) break;
                if (current_token(p).type == TOK_IDENTIFIER) {
                    if (peek_token(p, 1).type == TOK_COLON) {
                        //<param> ::= <unary_expression> ":" <unary_expression>
                        AST_typed_field curr_field = {0};
                        AST field = parse_unary_expr(p);
                        advance_token(p);
                        da_append(&n.as_fn_type_expr->returns, ((AST_typed_field){.field = field, .type = parse_unary_expr(p)}));
                        continue;
                    }
                    if (peek_token(p, 1).type == TOK_COMMA) { 
                        //<multi_param> ::= (<unary_expression>  ",")+ <unary_expression> ":" <unary_expression> 
                        AST_typed_field curr_field;
                        curr_field.field = parse_unary_expr(p);
                        curr_field.type.type = 0;
                        da_append(&n.as_fn_type_expr->returns, curr_field);
                        advance_token(p);
                        continue;
                    }
                }
                error_at_parser(p, "unexpected");
            }
            advance_token(p);
        } else {
            AST uexpr = parse_unary_expr(p);
            if (is_null_AST(uexpr)) error_at_parser(p, "expected type or identifier");
            AST_typed_field curr_field = {0};
            curr_field.type = uexpr;
            da_append(&n.as_fn_type_expr->returns, curr_field);
        }

    }
    n.as_fn_type_expr->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_unary_expr(parser* p) {
    /*<unary_expression> ::= <unop> <unary_expression> |
                       ("cast" | "bitcast") "(" <unary_expression> ")" <unary_expression> |
                       ("^" | "[]") ("mut" | "let") ( <unary_expression> | E) |
                       "[" <expression> "]" <unary_expression> | 
                       "distinct" <unary_expression> |
                       <atomic_expression>*/
    AST n = NULL_AST;

    //printf("current token: %s\n", token_type_str[current_token(p).type]);

    switch (current_token(p).type) {
        //<unop> <unary_expression>
        case TOK_AND:
        case TOK_SUB:
        case TOK_TILDE:
        case TOK_EXCLAM:
        case TOK_QUESTION:
        case TOK_KEYWORD_SIZEOF:
        case TOK_KEYWORD_ALIGNOF:
            n = new_ast_node(p, AST_unary_op_expr);
            n.as_binary_op_expr->base.start = &current_token(p);
            n.as_unary_op_expr->op = &current_token(p);
            advance_token(p);
            n.as_unary_op_expr->inside = parse_unary_expr(p);
            if (is_null_AST(n.as_unary_op_expr->inside)) error_at_parser(p, "expected unary expression");
            n.as_binary_op_expr->base.end = &peek_token(p, -1);
            return n;
        //("cast" | "bitcast") "(" <unary_expression> ")" <unary_expression>
        case TOK_KEYWORD_CAST:
        case TOK_KEYWORD_BITCAST:
            n = new_ast_node(p, AST_cast_expr);
            n.as_cast_expr->base.start = &current_token(p);
            n.as_cast_expr->is_bitcast = current_token(p).type == TOK_KEYWORD_BITCAST;
            advance_token(p);
            if (current_token(p).type != TOK_OPEN_PAREN) error_at_parser(p, "expected (");
            advance_token(p);
            n.as_cast_expr->type = parse_unary_expr(p);
            if (is_null_AST(n.as_cast_expr->type)) error_at_parser(p, "expected type");
            if (current_token(p).type != TOK_CLOSE_PAREN) error_at_parser(p, "expected )");
            n.as_cast_expr->base.end = &current_token(p);
            advance_token(p);
            n.as_cast_expr->rhs = parse_unary_expr(p);
            if (is_null_AST(n.as_cast_expr->rhs)) error_at_parser(p, "expected unary expression");
            return n;
        //"[" <expression> "]" <unary_expression>
        case TOK_OPEN_BRACKET:
            if (peek_token(p, 1).type != TOK_OPEN_BRACKET) {
                advance_token(p);
                n = new_ast_node(p, AST_array_type_expr);
                n.as_array_type_expr->base.start = &current_token(p);
                n.as_array_type_expr->length = parse_expr(p);
                if (current_token(p).type != TOK_CLOSE_BRACKET) error_at_parser(p, "expected ]");
                advance_token(p);
                n.as_array_type_expr->type = parse_unary_expr(p);
                n.as_array_type_expr->base.end = &current_token(p);
                return n;
            }
        //("^" | "[]") ("mut" | "let") ( <unary_expression> | E)
            advance_token(p);
            if (current_token(p).type != TOK_CLOSE_BRACKET) error_at_parser(p, "expected ]");
            n = new_ast_node(p, AST_slice_type_expr);
            n.as_slice_type_expr->base.start = &current_token(p);
        case TOK_CARET:
            if (is_null_AST(n)) n = new_ast_node(p, AST_pointer_type_expr);
            advance_token(p);
            if (current_token(p).type != TOK_KEYWORD_MUT && current_token(p).type != TOK_KEYWORD_LET) error_at_parser(p, "expected mut or let");
            n.as_pointer_type_expr->mutable = current_token(p).type == TOK_KEYWORD_MUT;
            advance_token(p);
            n.as_pointer_type_expr->subexpr = parse_unary_expr(p);
            n.as_pointer_type_expr->base.end = &current_token(p);
            return n;
        //"distinct" <unary_expression>
        case TOK_KEYWORD_DISTINCT:
            n = new_ast_node(p, AST_distinct_type_expr);
            n.as_distinct_type_expr->base.start = &current_token(p);
            advance_token(p);
            n.as_distinct_type_expr->subexpr = parse_unary_expr(p);
            n.as_distinct_type_expr->base.end = &current_token(p);
            return n;
        default:
            return parse_atomic_expr(p);
    }
}

AST parse_expr(parser* p) {
    //this is gonna be painful
    //we first need to detect WHICH expr branch we're on.
    //
    /*<expression> ::= <expression> <binop> <expression> | <unary_expression>*/

    //<unary_expression>
    AST n;

    n = parse_unary_expr(p);

    //<expression> <binop> <expression>
    if (verify_binop(p, current_token(p), false) == -1) return n;
    return parse_binop_expr(p, n, verify_binop(p, current_token(p), true));
}

AST parse_expr_wp(parser* p, int precedence) {
    AST n;
    n = parse_unary_expr(p);

    if (current_token(p).type == TOK_SEMICOLON) return n;
    return parse_binop_expr(p, n, precedence); 
}

AST parse_binop_expr(parser* p, AST lhs, int precedence) {
    //pratt parsing!
    //explanation: we keep tr
    //ack of a precedence p when we start.
    //we then parse, and if this precedence has changed, we break out of this loop 
    while (precedence < verify_binop(p, current_token(p), true)) {
        lhs = parse_binop_recurse(p, lhs, precedence);
    }

    return lhs;
}

AST parse_binop_recurse(parser* p, AST lhs, int precedence) {
    AST n = new_ast_node(p, AST_binary_op_expr);
    n.as_binary_op_expr->base.start = lhs.base->start;
    n.as_binary_op_expr->op = &current_token(p);
    n.as_binary_op_expr->lhs = lhs;
    //current_token is a binop here
    advance_token(p);

    n.as_binary_op_expr->rhs = parse_expr_wp(p, verify_binop(p, *n.as_binary_op_expr->op, true));
    n.as_binary_op_expr->base.end = n.as_binary_op_expr->rhs.base->end;
    return n;
}

int verify_binop(parser* p, token tok, bool error) {
    /*<binop> ::= "*" | "/" | "%" | "%%" | "&" | "<<" | ">>" | "~|" |
               "+" | "-" | "|" | "~" |
               "==" | "!=" | "<" | ">" | "<=" | ">=" |
               "&&" | "||" */
    switch (tok.type) {
        case TOK_MUL:       
        case TOK_DIV:       
        case TOK_MOD:       
        case TOK_MOD_MOD:       
        case TOK_AND: 
        case TOK_LSHIFT:    
        case TOK_RSHIFT:
        case TOK_NOR: 
            return 6;
        case TOK_ADD:
        case TOK_SUB:
        case TOK_OR:
        case TOK_TILDE:
            return 5;   
        case TOK_EQUAL_EQUAL:
        case TOK_NOT_EQUAL:
        case TOK_GREATER_THAN:
        case TOK_GREATER_EQUAL:
        case TOK_LESS_THAN:
        case TOK_LESS_EQUAL:
            return 4;
        case TOK_AND_AND:
            return 3;
        case TOK_OR_OR:
            return 2;
        default:
            if (error) error_at_parser(p, "expected binary operator");
            return -1;
    }
}

AST parse_decl_stmt(parser* p) {
    //<decl> ::= ("let" | "mut") (<identifier> ",")* <identifier> ("," | E) 
    //(":" <type> | E) "=" <r_value> ";"
    AST n = new_ast_node(p, AST_decl_stmt);
    n.as_decl_stmt->base.start = &current_token(p);
    da_init(&n.as_decl_stmt->lhs, 1);
    if (current_token(p).type == TOK_KEYWORD_MUT) n.as_decl_stmt->is_mut = 1;
    advance_token(p);
    
    if (current_token(p).type != TOK_IDENTIFIER) error_at_parser(p, "expected identifier when parsing lhs");
    while (current_token(p).type == TOK_IDENTIFIER) {
        AST ident = new_ast_node(p, AST_identifier);
        ident.as_identifier->base.start = &current_token(p);

        ident.as_identifier->tok = &current_token(p);
        da_append(&n.as_decl_stmt->lhs, ident);

        ident.as_identifier->base.end = &current_token(p);
        advance_token(p);
        if (current_token(p).type == TOK_COMMA) advance_token(p);
    } 

    if (current_token(p).type == TOK_COLON) {
        advance_token(p);
        if (current_token(p).type == TOK_EQUAL) error_at_parser(p, "expected type");
        n.as_decl_stmt->type = parse_unary_expr(p);
        advance_token(p);
    }
    if (current_token(p).type != TOK_EQUAL && current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected either = or ;");
    advance_token(p);
    if (current_token(p).type == TOK_UNINIT) n.as_decl_stmt->rhs = NULL_AST;
    else n.as_decl_stmt->rhs = parse_expr(p);
    n.as_decl_stmt->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_module_decl(parser* p) {
    //<module_stmt> ::= <ws> "module" <fws> <identifier> <ws> ";" <ws>
    AST n = new_ast_node(p, AST_module_decl);

    if (current_token(p).type != TOK_KEYWORD_MODULE)
        error_at_parser(p, "expected module declaration");

    n.as_module_decl->base.start = &current_token(p);

    advance_token(p);
    if (current_token(p).type != TOK_IDENTIFIER)
        error_at_parser(p, "expected module name");

    n.as_module_decl->name = &current_token(p);

    advance_token(p);
    if (current_token(p).type != TOK_SEMICOLON)
        error_at_parser(p, "expected ';'");
    
    n.as_module_decl->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_import_decl(parser* p) {
    AST n = new_ast_node(p, AST_import_stmt);
    n.as_import_stmt->base.start = &current_token(p);
    if (current_token(p).type == TOK_IDENTIFIER) {
        n.as_import_stmt->name = new_ast_node(p, AST_identifier);
        n.as_import_stmt->name.as_identifier->tok = &current_token(p);
        advance_token(p);
    }
    advance_token(p);

    if (current_token(p).type != TOK_LITERAL_STRING) error_at_parser(p, "expected string literal");
    n.as_import_stmt->path = new_ast_node(p, AST_literal_expr);
    n.as_import_stmt->path.as_literal_expr->tok = &current_token(p);
    advance_token(p);
    if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected semicolon");
    n.as_import_stmt->base.end = &current_token(p);
    advance_token(p);
    return n;
}