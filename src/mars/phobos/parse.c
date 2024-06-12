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
        
        general_error("Encountered unexpected token type %s", token_type_str[current_token(p).type]);
    }
}

AST parse_stmt(parser* p) {
    switch (current_token(p).type) {
        case TOK_KEYWORD_LET:
        case TOK_KEYWORD_MUT:
            parse_decl_stmt(p);
            break;
        default:
            error_at_parser(p, "found unexpected token during statement parsing");
    }
}

AST parse_atomic_expr(parser* p) {
    //this function handles explicitly atomic_expr that require left-assoc stuff.

/*<atomic_expression> ::= <atomic_expression> ("::" | "." | "->") <identifier> |
                        <atomic_expression> "[" <expression> "]" |
                        <atomic_expression> "[" (<expression> | E) ":" (<expression> | E) "]" |
                        <atomic_expression> "(" ((<expression> ",")* <expression> ("," | E) | E) ")" |
                        <atomic_expression> "^" |
                        <atomic_expression> "{" ((<expression> ",")* <expression> ("," | E) | E) "}"*/

    AST n = parse_atomic_expr_term(p);
    //TODO("non-term atomic_expr");
}

AST parse_atomic_expr_term(parser* p) {
    //this function is for <atomic_expression> bnf leafs with terminals explicitly in them
    //not left-assoc, basically
    /*<atomic_expression_terminals> ::= <literal> | <identifier> | "." <identifier> | <fn> | 
                        "i8" | "i16" | "i32" | "i64" | "int" |
                        "u8" | "u16" | "u32" | "u64" | "uint" |
                        "f16" | "f32" | "f64" | "float" | "bool" |
                        <fn_type> | <aggregate> | <enum> |
                        "(" <expression> ")"*/

    AST n;

    switch (current_token(p).type) {
        //<literal>
        case TOK_LITERAL_INT:
        case TOK_LITERAL_FLOAT:
        case TOK_LITERAL_BOOL:
        case TOK_LITERAL_STRING:
        case TOK_LITERAL_CHAR:
        case TOK_LITERAL_NULL:
            n = new_ast_node(p, AST_literal_expr);
            n.as_literal_expr->base.start = &current_token(p);
            n.as_literal_expr->tok = &current_token(p);
            n.as_literal_expr->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<identifier>
        case TOK_IDENTIFIER:
            n = new_ast_node(p, AST_identifier);
            n.as_identifier->base.start = &current_token(p);
            n.as_identifier->tok = &current_token(p);
            n.as_identifier->base.end = &current_token(p);
            advance_token(p);
            return n; 
        // "." <identifier> enum selector! this is for the semanal
        case TOK_PERIOD:
            n = new_ast_node(p, AST_selector_expr);
            n.as_selector_expr->base.start = &current_token(p);
            n.as_selector_expr->op = &current_token(p);
            advance_token(p);
            n.as_selector_expr->lhs = NULL_AST;
            n.as_selector_expr->rhs = new_ast_node(p, AST_identifier);
                n.as_selector_expr->rhs.as_identifier->base.start = &current_token(p);
                n.as_selector_expr->rhs.as_identifier->tok = &current_token(p);
                n.as_selector_expr->rhs.as_identifier->base.end = &current_token(p);
            n.as_identifier->base.end = &current_token(p);
            advance_token(p);
            return n;            
        //<fn>
        case TOK_KEYWORD_FN:
            return parse_fn_type(p);
        //"i8" | "i16" | "i32" | "i64" | "int" |
        //"u8" | "u16" | "u32" | "u64" | "uint" |
        //"f16" | "f32" | "f64" | "float" | "bool" |
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
        case TOK_TYPE_KEYWORD_BOOL:
            n = new_ast_node(p, AST_basic_type_expr);
            n.as_basic_type_expr->base.start = &current_token(p);
            n.as_basic_type_expr->lit = &current_token(p);
            printf("curr_tok: %s, 0x%08x\n", token_type_str[current_token(p).type], &current_token(p));
            n.as_basic_type_expr->base.end = &current_token(p);
            advance_token(p);
            return n;
        default:
            error_at_parser(p, "unknown token type %s", token_type_str[current_token(p).type]);
    }
    return NULL_AST;
}

AST parse_fn_type(parser* p) {
    /*<fn_type> ::= "fn" "(" (<param_list> ("," | E) | E) ")" 
     ("->" (<unary_expression> | "(" <param_list> ")") | E)

    <param> ::= <identifier> ":" <unary_expression> 
    <multi_param> ::= (<identifier>  ",")+ <identifier> ":" <unary_expression> 
    <param_list> ::= <param_list> "," <param> | 
                     <param_list> "," <multi_param> | 
                     <param> | <multi_param>*/

    AST n = new_ast_node(p, AST_fn_type_expr);
    n.as_fn_type_expr->base.start = &current_token(p);
    da_init(&n.as_fn_type_expr->parameters, 1);
    advance_token(p);
    if (current_token(p).type != TOK_OPEN_PAREN) error_at_parser(p, "expected (");
    advance_token(p);

    //fn(a, b, c: int, d: i32)

    if (current_token(p).type != TOK_CLOSE_PAREN) { 
        //param parsing
        for (int i = p->current_tok; i < p->tokens.len; i++) {
            //printf("current tok: "str_fmt", type: %s\n", str_arg(current_token(p).text), token_type_str[current_token(p).type]);
            if (current_token(p).type == TOK_COLON) error_at_parser(p, "unexpected :");
            if (current_token(p).type == TOK_COMMA) advance_token(p);
            if (current_token(p).type == TOK_CLOSE_PAREN) break;
            if (current_token(p).type == TOK_IDENTIFIER) {
                if (peek_token(p, 1).type == TOK_COLON) {
                    //<param> ::= <identifier> ":" <unary_expression> 
                    AST_typed_field curr_field = {0};
                    //printf("current tok: "str_fmt", type: %s\n", str_arg(current_token(p).text), token_type_str[current_token(p).type]);
                    curr_field.field = new_ast_node(p, AST_identifier);
                    curr_field.field.as_identifier->base.start = &current_token(p);
                    curr_field.field.as_identifier->tok = &current_token(p);
                    curr_field.field.as_identifier->base.end = &current_token(p);
                    advance_token(p); advance_token(p);
                   // printf("current tok: "str_fmt", type: %s\n\n", str_arg(current_token(p).text), token_type_str[current_token(p).type]);
                    curr_field.type = parse_unary_expr(p);
                    da_append(&n.as_fn_type_expr->parameters, curr_field);
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA) { 
                    //<multi_param> ::= (<identifier>  ",")+ <identifier> ":" <unary_expression>
                    AST_typed_field curr_field;
                    curr_field.field = new_ast_node(p, AST_identifier);
                    curr_field.field.as_identifier->base.start = &current_token(p);
                    curr_field.field.as_identifier->tok = &current_token(p);
                    curr_field.field.as_identifier->base.end = &current_token(p);
                    curr_field.type.type = 0;
                    da_append(&n.as_fn_type_expr->parameters, curr_field);
                    advance_token(p); advance_token(p);
                    continue;
                }
            }
            //advance_token(p);
            error_at_parser(p, "unexpected");
        }
    } else {
        n.as_fn_type_expr->parameters.at = NULL;
    }
    if (current_token(p).type != TOK_CLOSE_PAREN) error_at_parser(p, "expected )");
    for (int i = 0; i < n.as_fn_type_expr->parameters.len; i++) {
        AST_typed_field tf = n.as_fn_type_expr->parameters.at[i];
        printf("field: "str_fmt", type: "str_fmt", addr: 0x%08x\n", 
            str_arg(tf.field.as_identifier->tok->text), 
            str_arg(tf.type.as_basic_type_expr->lit->text), 
            &tf.type.as_basic_type_expr->lit->text);
    }
    advance_token(p);
    if (current_token(p).type == TOK_ARROW_RIGHT) {
        //returns time!
    }
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
            if (current_token(p).type != TOK_KEYWORD_MUT || current_token(p).type != TOK_KEYWORD_LET) error_at_parser(p, "expected mut or let");
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
}