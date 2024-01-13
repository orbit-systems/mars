#include "orbit.h"
#include "error.h"

#include "parser.h"
#include "ast.h"

#define error_at_parser(p, message, ...) \
    error_at_string(p->path, p->src, current_token(p).text, message __VA_OPT__(,) __VA_ARGS__)

// construct a parser struct from a lexer and an arena allocator
parser make_parser(lexer* restrict l, arena alloca) {
    parser p = {0};
    p.alloca = alloca;
    p.tokens = l->buffer;
    p.path   = l->path;
    p.src    = l->src;
    p.current_token = 0;

    return p;
}

AST parse_module_decl(parser* restrict p) {

    AST n = new_ast_node(&p->alloca, astype_module_decl);

    if (current_token(p).type != tt_keyword_module)
        error_at_parser(p, "expected module declaration, got %s", token_type_str[current_token(p).type]);
    n.as_module_decl->base.start = &current_token(p);

    advance_token(p);
    if (current_token(p).type != tt_identifier)
        error_at_parser(p, "expected module name, got %s", token_type_str[current_token(p).type]);

    n.as_module_decl->name = &current_token(p);

    advance_token(p);
    if (current_token(p).type != tt_semicolon)
        error_at_parser(p, "expected ';', got %s", token_type_str[current_token(p).type]);
    
    n.as_module_decl->base.end = &current_token(p);
    advance_token(p);
    return n;
}

void parse_file(parser* restrict p) {
    
    p->module_decl = parse_module_decl(p);

    p->head = new_ast_node(&p->alloca, astype_block_stmt);

}

AST parse_expr(parser* restrict p) {
    AST n = {0};

    switch (current_token(p).type) {
    case tt_open_paren:
        n = new_ast_node(&p->alloca, astype_paren_expr);
        
        n.as_paren_expr->base.start = &current_token(p); 
        
        advance_token(p);
        n.as_paren_expr->subexpr = parse_expr(p);
        if (current_token(p).type != tt_close_paren)
            error_at_parser(p, "expected ')', got %s", token_type_str[current_token(p).type]);
        
        n.as_paren_expr->base.end = &current_token(p);
        advance_token(p);
        break;

    default:
        TODO("probably unimplemented some shit");
        break;
    }
    return n;
}

AST parse_stmt(parser* restrict p, bool require_semicolon) {
    AST n = NULL_AST;

    switch (current_token(p).type) {
    case tt_keyword_if:
        n = new_ast_node(&p->alloca, astype_if_stmt);
        n.as_if_stmt->is_elif = false;
        n.as_if_stmt->base.start = &current_token(p);
        advance_token(p);

        n.as_if_stmt->condition = parse_expr(p);
        n.as_if_stmt->if_branch = parse_block_stmt(p);
        
        n.as_if_stmt->base.end = &peek_token(p, -1);

        if (current_token(p).type == tt_keyword_elif) {
            n.as_if_stmt->else_branch = parse_elif(p);
        }
        else if (current_token(p).type == tt_keyword_else) {
            advance_token(p);
            n.as_if_stmt->else_branch = parse_block_stmt(p);
        }
        break;
    case tt_keyword_while:
        n = new_ast_node(&p->alloca, astype_while_stmt);
        n.as_while_stmt->base.start = &current_token(p);
        advance_token(p);

        n.as_while_stmt->condition = parse_expr(p);
        if (n.as_while_stmt->condition.rawptr == NULL) {

        }
        n.as_while_stmt->block = parse_block_stmt(p);

        n.as_while_stmt->base.end = &peek_token(p, -1);
        break;
    case tt_keyword_for:
        // jank as shit but whatever
        if ((peek_token(p, 1).type == tt_identifier || peek_token(p, 1).type == tt_identifier_discard) &&
        (peek_token(p, 2).type == tt_colon || peek_token(p, 2).type == tt_keyword_in)) {
            // for-in loop!
            n = new_ast_node(&p->alloca, astype_for_in_stmt);
            n.as_for_in_stmt->base.start = &current_token(p);
            advance_token(p);

            n.as_for_in_stmt->indexvar = parse_expr(p);

            if (current_token(p).type == tt_colon) {
                advance_token(p);
                n.as_for_in_stmt->type = parse_expr(p);
                if (n.as_for_in_stmt->type.rawptr == NULL)
                    error_at_parser(p, "expected type expression after ':'");
            }

            if (current_token(p).type != tt_keyword_in)
                error_at_parser(p, "expected 'in', got '%s'", token_type_str[current_token(p).type]);

            advance_token(p);

            n.as_for_in_stmt->from = parse_expr(p);
            if (current_token(p).type == tt_range_less) {
                n.as_for_in_stmt->is_inclusive = false;
            } else if (current_token(p).type == tt_range_eq) {
                n.as_for_in_stmt->is_inclusive = true;
            } else {
                error_at_parser(p, "expected '..<' or '..=', got '%s'", token_type_str[current_token(p).type]);
            }
            advance_token(p);
            
            n.as_for_in_stmt->to = parse_expr(p);

            n.as_for_in_stmt->block = parse_block_stmt(p);

            n.as_for_in_stmt->base.end = &peek_token(p, -1);
        } else {
            // regular for loop!
            TODO("regular for loop parsing");
            n = new_ast_node(&p->alloca, astype_for_stmt);
            n.base->start = &current_token();
            advance_token(p);

            n.as_for_stmt->prelude = parse_stmt(p, true);
            
            n.as_for_stmt->condition = parse_expr(p);
            if (current_token(p).type != tt_semicolon)
                error_at_parser(p, "expected ';', got '%s'", token_type_str[current_token(p).type]);

            advance_token(p);
            n.as_for_stmt->post_stmt = parse_stmt(p, false);
            
            n.as_for_stmt->block = parse_block_stmt(p);

            n.base->end = &peek_token(p, -1);
        }
        break;
    case tt_semicolon:
        n = new_ast_node(&p->alloca, astype_empty_stmt);
        n.as_empty_stmt->base.start = &current_token(p);
        n.as_empty_stmt->base.end = &current_token(p);
        break;
    case tt_open_bracket:
        n = parse_block_stmt(p);
        break;
    
    default:
        TODO("probably unimplemented");
        break;
    }
    return n;
}

// only for use within parse_stmt
AST parse_elif(parser* restrict p) {
    AST n = new_ast_node(&p->alloca, astype_if_stmt);
    n.as_if_stmt->is_elif = true;
    n.as_if_stmt->base.start = &current_token(p);

    advance_token(p);

    // get condition expression
    n.as_if_stmt->condition = parse_expr(p);
        
    // get block
    n.as_if_stmt->if_branch = parse_block_stmt(p);

    n.as_if_stmt->base.end = &peek_token(p, -1);

    if (current_token(p).type == tt_keyword_elif) {
        n.as_if_stmt->else_branch = parse_elif(p);
    }
    else if (current_token(p).type == tt_keyword_else) {
        advance_token(p);
        n.as_if_stmt->else_branch = parse_block_stmt(p);
    }
    return n;
}

AST parse_block_stmt(parser* restrict p) {
    AST n = new_ast_node(&p->alloca, astype_block_stmt);
    
    if (current_token(p).type != tt_open_brace)
        error_at_parser(p, "expected '{', got '%s'", token_type_str[current_token(p).type]);
    n.as_block_stmt->base.start = &current_token(p);
    
    advance_token(p);
    while (current_token(p).type != tt_close_brace) {
        AST stmt = parse_stmt(p, true);
        dynarr_append(AST, &n.as_block_stmt->stmts, stmt);
    }

    if (current_token(p).type != tt_close_brace)
        error_at_parser(p, "expected '}', got '%s'", token_type_str[current_token(p).type]);
    n.as_block_stmt->base.end = &current_token(p);

    return n;
}