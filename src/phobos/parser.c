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
    AST smth = parse_stmt(p, true);
}

AST parse_expr(parser* restrict p) {
    AST n = {0};

    switch (current_token.type) {
    case tt_open_paren:
        n = new_ast_node(&p->alloca, astype_paren_expr);
        
        n.as_paren_expr->base.start = &current_token; 
        
        advance_token;
        n.as_paren_expr->subexpr = parse_expr(p);
        if (current_token.type != tt_close_paren)
            error_at_parser(p, "expected ')', got %s", token_type_str[current_token.type]);
        
        n.as_paren_expr->base.end = &current_token;
        advance_token;
        break;
    default:
        TODO("parse_expr() unimplemented");
        break;
    }
    return n;
}

AST parse_import_stmt(parser* restrict p) {

}

AST parse_stmt(parser* restrict p, bool expect_semicolon) {
    AST n = NULL_AST;

    switch (current_token.type) {
    case tt_keyword_defer:
        n = new_ast_node(&p->alloca, astype_defer_stmt);
        n.as_defer_stmt->base.start = &current_token;
        advance_token;

        n.as_defer_stmt->stmt = parse_stmt(p, expect_semicolon);

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

            n.as_for_stmt->prelude = parse_stmt(p, true);
            
            n.as_for_stmt->condition = parse_expr(p);
            if (current_token.type != tt_semicolon)
                error_at_parser(p, "expected ';', got '%s'", token_type_str[current_token.type]);

            advance_token;
            n.as_for_stmt->post_stmt = parse_stmt(p, false);
            
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

        if (expect_semicolon && current_token.type != tt_semicolon)
            error_at_parser(p, "expected ';' after type declaration", token_type_str[current_token.type]);
        
        advance_token;
        n.as_type_decl_stmt->base.end = &peek_token(-1);
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
                error_at_parser(p, "invalid directive \'%s\' for declaration statement", clone_to_cstring(current_token.text));
            }
        }

        // parse identifier list
        dynarr_init(AST, &n.as_decl_stmt, 4); // four is probably good right
        


        TODO("parse_stmt() declarations");
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
        AST stmt = parse_stmt(p, true);
        dynarr_append(AST, &n.as_block_stmt->stmts, stmt);
    }

    if (current_token.type != tt_close_brace)
        error_at_parser(p, "expected '}', got '%s'", token_type_str[current_token.type]);
    n.as_block_stmt->base.end = &current_token;

    return n;
}