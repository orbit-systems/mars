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

    /*switch (current_token(p).type) {
        //<unop> <expression>
        case TOK_AMPERSAND:
        case TOK_SUB:
        case TOK_TILDE:
        case TOK_EXCLAM:
        case TOK_QUESTION:
            n = new_ast_node(p, AST_UNARY_OP);
            n.as_unary_op_expr->op = &current_token(p);
            n.as_unary_op_expr->inside = parse_expr(p);
            advance_token(p);
            break;
        //<atomic_expression>
    }*/

AST parse_type(parser* p) {
    TODO("AGUHGHFJKKKL");
}

AST parse_expr(parser* p) {
    //this is gonna be painful
    //we first need to detect WHICH expr branch we're on.
    //
    /*<expression> ::= <expression> <binop> <expression> | <unary_expression>*/

    //<unary_expression>
    AST n;

    n = parse_unary_expr(p);
    advance_token(p);

    //<expression> <binop> <expression>
    if (current_token(p).type == TOK_SEMICOLON) return n;
    return parse_binop_expr(p, n, verify_binop(p, current_token(p)));
}

AST parse_expr_wp(parser* p, int precedence) {
    AST n;
    n = parse_unary_expr(p);
    advance_token(p);

    if (current_token(p).type == TOK_SEMICOLON) return n;
    return parse_binop_expr(p, n, precedence); 
}

AST parse_binop_expr(parser* p, AST lhs, int precedence) {
    //pratt parsing!
    //explanation: we keep track of a precedence p when we start.
    //we then parse, and if this precedence has changed, we break out of this loop 
    while (precedence < verify_binop(p, current_token(p))) {
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

    n.as_binary_op_expr->rhs = parse_expr_wp(p, verify_binop(p, *n.as_binary_op_expr->op));
    n.as_binary_op_expr->base.end = n.as_binary_op_expr->rhs.base->end;
    return n;
}

AST parse_unary_expr(parser* p) {
    TODO("unary expr");
}

int verify_binop(parser* p, token tok) {
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
            error_at_parser(p, "expected binary operator");
    }
}

AST parse_decl_stmt(parser* p) {
    //<decl> ::= <decl> ::= ("let" | "mut") (<identifier> ",")* <identifier> ("," | E) 
    //(":" <type> | E) "=" <r_value> ";"
    AST n = new_ast_node(p, AST_decl_stmt);
    da_init(&n.as_decl_stmt->lhs, 1);
    if (current_token(p).type == TOK_KEYWORD_MUT) n.as_decl_stmt->is_mut = 1;
    advance_token(p);
    
    if (current_token(p).type != TOK_IDENTIFIER) error_at_parser(p, "expected identifier when parsing lhs");
    while (current_token(p).type == TOK_IDENTIFIER) {
        AST ident = new_ast_node(p, AST_identifier);
        ident.as_identifier->tok = &current_token(p);
        da_append(&n.as_decl_stmt->lhs, ident);
        advance_token(p);
        if (current_token(p).type == TOK_COMMA) advance_token(p);
    } 

    if (current_token(p).type == TOK_COLON) {
        advance_token(p);
        if (current_token(p).type == TOK_EQUAL) error_at_parser(p, "expected type");
        n.as_decl_stmt->type = parse_type(p);
        advance_token(p);
    }
    if (current_token(p).type != TOK_EQUAL && current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected either = or ;");
    advance_token(p);
    if (current_token(p).type == TOK_UNINIT) n.as_decl_stmt->rhs = NULL_AST;
    else n.as_decl_stmt->rhs = parse_expr(p);
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
    advance_token(p);
}