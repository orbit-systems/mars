#include "common/orbit.h"
#include "mars/term.h"

#include "parse.h"

// kayla's FREEZINGLY 🧊🧊 SLOW 🐌🐌 parser in ALGOL69 👴👴 + KITTY POWERED with KIBBLE 5.0 🐈🐈 and DOGECOIN WEB4 TECHNOLOGY
// predictive LL(1) parser using the ebnf spec

/*
    README FOR THOSE WANTING TO MODIFY THIS:
    functions of the form parse_***(parser* p) will advance the token stream for you
    if you have any questions, please contact kaylatheegg on discord or leave a github
    issue and i'll give you alternative contact info if you do not have discord.
 */

// #define debug_trace(p) printf("stack -> %s @ %zu '" str_fmt "'\n", __func__, (p)->current_tok, str_arg((p)->tokens.at[(p)->current_tok].text))
#define debug_trace(p)

// construct a parser struct from a lexer and an arena allocator
parser make_parser(lexer* l, Arena* alloca) {
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
    debug_trace(p);
    //<program> ::= <module_stmt> <import_stmt>* <stmts>
    p->module_decl = parse_module_decl(p);

    general_warning("NOTE: this compiler is susceptible to just. falling into infinite loops");
    general_warning("      this is because it fails to match, and cannot figure out how to  ");
    general_warning("      escape safely. please report any you find!\n");
    
    general_warning("CURRENTLY KNOWN INFINITE LOOPS:");
    general_warning("please expand this list");

    da_init(&p->stmts, 1);

    while (current_token(p).type == TOK_KEYWORD_IMPORT) da_append(&p->stmts, parse_import_decl(p));

    while (current_token(p).type != TOK_EOF) {
        da_append(&p->stmts, parse_stmt(p));
        
        //general_error("Encountered unexpected token type %s", token_type_str[current_token(p).type]);
    }
}

AST parse_stmt(parser* p) {
    debug_trace(p);
    /*<stmt> ::= <decl> | <return> | <if> | <else> | <while> | 
           <for_type_one> | <for_type_two> | <asm> | <stmt_block> |
           <switch> | <break> | <fallthrough> | <expression> ";" | <extern> | <defer> | <continue> | ";"
            <assignment> | <type_decl> */
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
                if (current_token(p).type == TOK_SEMICOLON) break;
                if (current_token(p).type != TOK_COMMA) error_at_parser(p, "expected ,");
                advance_token(p);
            }
            n.as_return_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<if> ::= "if" <expression> <control_flow_block>
        case TOK_KEYWORD_IF:
            n = new_ast_node(p, AST_if_stmt);
            n.as_if_stmt->base.start = &current_token(p);
            advance_token(p);
            n.as_if_stmt->condition = parse_expr(p);

            n.as_if_stmt->if_branch = parse_cfs(p);
            n.as_if_stmt->base.end = &current_token(p);
            return n;
        //<else> ::= "else" (<if> | <stmt_block>)
        case TOK_KEYWORD_ELSE:
            n = new_ast_node(p, AST_else_stmt);
            n.as_else_stmt->base.start = &current_token(p);
            advance_token(p);
            if (current_token(p).type == TOK_KEYWORD_IF) {
                advance_token(p);
                n.as_else_stmt->inside = parse_stmt(p);
                return n;
            }
            n.as_else_stmt->inside = parse_stmt_block(p);
            return n;
        //<while> ::= "while" (<expression>  | E) <control_flow_block>
        case TOK_KEYWORD_WHILE:
            n = new_ast_node(p, AST_while_stmt);
            n.as_while_stmt->base.start = &current_token(p);
            advance_token(p);
            if (current_token(p).type == TOK_KEYWORD_DO) {
                advance_token(p);
                n.as_while_stmt->condition = NULL_AST;
                n.as_while_stmt->block = parse_stmt(p);
                n.as_while_stmt->base.end = &current_token(p);
                return n;
            }
            if (current_token(p).type == TOK_OPEN_BRACE) {
                n.as_while_stmt->condition = NULL_AST;
                n.as_while_stmt->block = parse_stmt_block(p);
                n.as_while_stmt->base.end = &current_token(p);
                return n;
            }
            n.as_while_stmt->condition = parse_expr(p);
            if (current_token(p).type == TOK_KEYWORD_DO) {
                advance_token(p);
                n.as_while_stmt->block = parse_stmt(p);
                n.as_while_stmt->base.end = &current_token(p);
                return n;
            }
            if (current_token(p).type == TOK_OPEN_BRACE) {
                n.as_while_stmt->block = parse_stmt_block(p);
                n.as_while_stmt->base.end = &current_token(p);
                return n;
            }
            error_at_parser(p, "expected do <stmt> or statement block");
        //<for_type_one> ::= "for" <simple_stmt> ";" <expression>  ";" (( <assignment> "," )* <assignment> ("," | E) | E) <control_flow_block>
        //<for_type_two> ::= "for" <identifier> (":" <type> | E) "in" <expression>  ("..<" | "..=")  <expression> <control_flow_block>
        case TOK_KEYWORD_FOR:
            // we need to look ahead here
            advance_token(p);
            if (peek_token(p, 1).type == TOK_COLON || peek_token(p, 1).type == TOK_KEYWORD_IN) {
                //<for_type_two> ::= "for" <identifier> (":" <type> | E) "in" <expression>  ("..<" | "..=") <expression> <control_flow_block>        
                general_warning("for-in loop detected! this code is unchecked.");
                n = new_ast_node(p, AST_for_in_stmt);
                n.as_for_in_stmt->base.start = &current_token(p);
                n.as_for_in_stmt->indexvar = parse_identifier(p);
                if (current_token(p).type == TOK_COLON) {
                    advance_token(p);
                    n.as_for_in_stmt->type = parse_unary_expr(p);
                }
                if (current_token(p).type != TOK_KEYWORD_IN) error_at_parser(p, "expected \"in\"");
                advance_token(p);
                n.as_for_in_stmt->from = parse_expr(p);
                if (current_token(p).type != TOK_RANGE_LESS && current_token(p).type != TOK_RANGE_EQ) error_at_parser(p, "expected ..< or ..=");
                n.as_for_in_stmt->is_inclusive = current_token(p).type == TOK_RANGE_EQ;
                advance_token(p);
                n.as_for_in_stmt->to = parse_expr(p);
                n.as_for_in_stmt->block = parse_cfs(p);
                n.as_for_in_stmt->base.end = &current_token(p);
                return n;
            }
            //<for_type_one> ::= "for" <simple_stmt> ";" <expression>  ";" (( <assignment> "," )* <assignment> ("," | E) | E) <control_flow_block>
            AST n = new_ast_node(p, AST_for_stmt);
            n.as_for_stmt->base.start = &current_token(p);
            if (current_token(p).type == TOK_KEYWORD_MUT || current_token(p).type == TOK_KEYWORD_LET) {
                //we have a decl!
                n.as_for_stmt->prelude = parse_decl_stmt(p);
            } else {
                //we have an expr? its assign OR expr, so if its an ident,
                //we look for a comma or equals sign. if either exist? assign. if neither? expr.
                if (current_token(p).type == TOK_IDENTIFIER) {
                    n.as_for_stmt->prelude = parse_stmt(p); //expr stmt goes in here! :)
                } else error_at_parser(p, "expected declaration, assignment or expression");
            }
            //we're now at expr parsing.
            //we've presumably skipped the semicolon here, evaluate this assumption
            n.as_for_stmt->condition = parse_expr(p);
            if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
            advance_token(p);
            //we're at the assignment list! we need to capture until cur_tok = TOK_OPEN_BRACE or TOK_KEYWORD_DO
            da_init(&n.as_for_stmt->update, 1);
            while (current_token(p).type != TOK_OPEN_BRACE && current_token(p).type != TOK_KEYWORD_DO) {
                da_append(&n.as_for_stmt->update, parse_stmt(p));
                if (current_token(p).type == TOK_COMMA) advance_token(p);
            }
            n.as_for_stmt->block = parse_cfs(p);
            n.as_for_stmt->base.end = &current_token(p);
            return n;
        //<asm> ::= "asm" "(" (<asm_param> ",")* <asm_param> ("," | E) ")" 
        // "{" (<string> ",")* <string> ("," | E) "}" 
        case TOK_KEYWORD_ASM:
            //this is going to suck. bad.
            //<asm_param> ::= <identifier> ("->" | "<-" | "<->") <string> 
            general_warning("asm blocks have not yet been tested! beware, there be demons in they/them hills over their");
            n = new_ast_node(p, AST_asm_stmt);
            n.as_asm_stmt->base.start = &current_token(p);
            advance_token(p);
            da_init(&n.as_asm_stmt->params, 1);
            da_init(&n.as_asm_stmt->strs, 1);
            if (current_token(p).type != TOK_OPEN_PAREN) error_at_parser(p, "expected (");
            advance_token(p);
            while (current_token(p).type != TOK_CLOSE_PAREN) {
                //scan for asm param!
                AST param = new_ast_node(p, AST_asm_param);
                param.as_asm_param->base.start = &current_token(p);
                if (current_token(p).type != TOK_IDENTIFIER) error_at_parser(p, "expected identifier");
                param.as_asm_param->ident = parse_identifier(p);
                if (current_token(p).type != TOK_ARROW_LEFT && current_token(p).type != TOK_ARROW_RIGHT && current_token(p).type != TOK_ARROW_BIDIR) error_at_parser(p, "expected <- or -> or <->");
                param.as_asm_param->op = &current_token(p);
                advance_token(p);
                if (current_token(p).type != TOK_LITERAL_STRING) error_at_parser(p, "expected string");
                param.as_asm_param->reg = parse_atomic_expr_term(p); //less code dup!
                param.as_asm_param->base.start = &current_token(p);
                da_append(&n.as_asm_stmt->params, param);
                if (current_token(p).type == TOK_COMMA) advance_token(p);
            }
            advance_token(p);
            if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");
            advance_token(p);
            while (current_token(p).type != TOK_CLOSE_BRACE) {
                da_append(&n.as_asm_stmt->strs, parse_atomic_expr_term(p));
                if (current_token(p).type == TOK_COMMA) advance_token(p);
            }
            n.as_asm_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<stmt_block>
        case TOK_OPEN_BRACE:
            return parse_stmt_block(p);
        /*<switch> ::= "switch" <expression> "{" 
             ("case" (<expression> ",")* <expression> ("," | E))* ":" <stmt>+
             ("case" ":" <stmt>+ )* "}" */
        case TOK_KEYWORD_SWITCH:
            general_warning("HEY! we havent yet tested switch parsing. there be demons here");
            //TODO: cases might overflow with their base.ends?
            n = new_ast_node(p, AST_switch_stmt);
            n.as_switch_stmt->base.start = &current_token(p);
            advance_token(p);
            n.as_switch_stmt->expr = parse_expr(p);
            if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");
            advance_token(p);
            if (current_token(p).type != TOK_KEYWORD_CASE) error_at_parser(p, "switch statements require at least one case");
            while (current_token(p).type != TOK_CLOSE_BRACE) {
                AST switch_case = new_ast_node(p, AST_case);
                switch_case.as_case->base.start = &current_token(p);
                da_init(&switch_case.as_case->matches, 1);
                advance_token(p);
                if (current_token(p).type != TOK_COLON) {
                    //we're in the match zone baybee
                    while (current_token(p).type != TOK_COLON) {
                        da_append(&switch_case.as_case->matches, parse_expr(p));
                        if (current_token(p).type == TOK_COMMA) advance_token(p);
                    }
                    //we're out of the match zone baybee
                }
                advance_token(p);

                AST stmt_block = new_ast_node(p, AST_stmt_block);
                stmt_block.as_stmt_block->base.start = &current_token(p);
                da_init(&stmt_block.as_stmt_block->stmts, 1);
                while (current_token(p).type != TOK_KEYWORD_CASE && current_token(p).type != TOK_CLOSE_BRACE) {
                    da_append(&stmt_block.as_stmt_block->stmts, parse_stmt(p));
                }
                stmt_block.as_stmt_block->base.end = &current_token(p);
                switch_case.as_case->block = stmt_block;
                switch_case.as_case->base.end = &current_token(p);
                da_append(&n.as_switch_stmt->cases, switch_case);
            }
            n.as_switch_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<break> ::= "break" (<identifier> | E) ";" 
        case TOK_KEYWORD_BREAK:
            n = new_ast_node(p, AST_break_stmt);
            n.as_break_stmt->base.start = &current_token(p);
            advance_token(p);
            if (current_token(p).type != TOK_SEMICOLON) {
                n.as_break_stmt->label = parse_atomic_expr_term(p);
            }
            if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
            n.as_break_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<fallthrough> ::= "fallthrough" ";" 
        case TOK_KEYWORD_FALLTHROUGH:
            n = new_ast_node(p, AST_fallthrough_stmt);
            n.as_fallthrough_stmt->base.start = &current_token(p);
            advance_token(p);
            if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
            n.as_fallthrough_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<extern> ::= "extern" <stmt>
        case TOK_KEYWORD_EXTERN:
            n = new_ast_node(p, AST_extern_stmt);
            n.as_extern_stmt->base.start = &current_token(p);
            advance_token(p);
            n.as_extern_stmt->stmt = parse_stmt(p);
            n.as_extern_stmt->base.end = &current_token(p);
            return n;
        //<defer> ::= "defer" <stmt>
        case TOK_KEYWORD_DEFER:
            n = new_ast_node(p, AST_defer_stmt);
            n.as_defer_stmt->base.start = &current_token(p);
            advance_token(p);
            n.as_defer_stmt->stmt = parse_stmt(p);
            n.as_defer_stmt->base.end = &current_token(p);
            return n;
        //<continue> ::= "continue" (<identifier> | E) ";"
        case TOK_KEYWORD_CONTINUE:
            n = new_ast_node(p, AST_continue_stmt);
            n.as_continue_stmt->base.start = &current_token(p);
            advance_token(p);
            if (current_token(p).type != TOK_SEMICOLON) {
                n.as_continue_stmt->label = parse_atomic_expr_term(p);
            }
            if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
            n.as_continue_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        case TOK_SEMICOLON:
            n = new_ast_node(p, AST_empty_stmt);
            n.as_empty_stmt->tok = &current_token(p);
            n.as_empty_stmt->base.start = &current_token(p);
            n.as_empty_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        //<type_decl> ::= "type" <identifier> "=" <type> ";"
        case TOK_KEYWORD_TYPE: 
            n = new_ast_node(p, AST_type_decl_stmt);
            n.as_type_decl_stmt->base.start = &current_token(p);
            advance_token(p);
            n.as_type_decl_stmt->lhs = parse_identifier(p);
            if (current_token(p).type != TOK_EQUAL) error_at_parser(p, "expected =");
            advance_token(p);
            n.as_type_decl_stmt->rhs = parse_type(p);
            if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
            n.as_type_decl_stmt->base.end = &current_token(p);
            advance_token(p);
            return n;
        default:
            token* curr_tok = &current_token(p);
            AST expr = parse_expr(p);
            if (is_null_AST(expr)) return NULL_AST;
            if (current_token(p).type == TOK_COMMA || verify_assign_op(p)) {
                //<assignment> ::= (<unary_expression> ",")* <unary_expression> <assign_op> (<expression> | "---")
                n = new_ast_node(p, AST_assign_stmt);
                n.as_assign_stmt->base.start = curr_tok;
                da_init(&n.as_assign_stmt->lhs, 1);
                da_append(&n.as_assign_stmt->lhs, expr);
                if (current_token(p).type == TOK_COMMA) advance_token(p);
                while (!verify_assign_op(p)) {
                    da_append(&n.as_assign_stmt->lhs, parse_unary_expr(p));
                    if (current_token(p).type == TOK_COMMA) advance_token(p);
                }
                n.as_assign_stmt->op = &current_token(p);
                advance_token(p);
                if (current_token(p).type != TOK_UNINIT) n.as_assign_stmt->rhs = parse_expr(p);
                else {
                    n.as_assign_stmt->rhs = NULL_AST;
                    advance_token(p);
                }
                if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
                n.as_assign_stmt->base.end = &current_token(p);
                advance_token(p);
                return n;
            } 
            //<expression> ";"
            if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
            advance_token(p);
            return expr;

    }
}

AST parse_cfs(parser* p) {
    debug_trace(p);
    AST n = new_ast_node(p, AST_stmt_block);
    n.as_stmt_block->base.start = &current_token(p);
    da_init(&n.as_stmt_block->stmts, 1);
    if (current_token(p).type == TOK_KEYWORD_DO) {
        advance_token(p);
        da_append(&n.as_stmt_block->stmts, parse_stmt(p));
        n.as_stmt_block->base.end = &current_token(p);
        return n;
    }    
    n = parse_stmt_block(p);
    n.as_stmt_block->base.end = &current_token(p);
    return n;
}

int verify_assign_op(parser* p) {
    //<assign_op> ::= "+=" | "-=" | "*="  | "/=" | "%="  | "%%=" | "&=" | "|=" | "~|=" | "~=" | "<<=" | ">>=" | "="
    switch (current_token(p).type) {
        case TOK_ADD_EQUAL:
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
        case TOK_RSHIFT_EQUAL:
        case TOK_EQUAL:
            return 1;
        default:
            return 0;
    }
}

AST parse_identifier(parser* p) {
    debug_trace(p);
    AST n = new_ast_node(p, AST_identifier);
    n.as_identifier->base.start = &current_token(p);
    n.as_identifier->tok = &current_token(p);
    n.as_identifier->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_type(parser* p) {
    debug_trace(p);
    AST n = NULL_AST;
    switch (current_token(p).type) {
        //<identifier>
        case TOK_IDENTIFIER:
            return parse_identifier(p); 
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
            n.as_basic_type_expr->base.end = &current_token(p);
            advance_token(p);
            return n;
        case TOK_KEYWORD_STRUCT:
        case TOK_KEYWORD_UNION:
            return parse_aggregate(p);
        case TOK_KEYWORD_ENUM:
            return parse_enum(p);
        case TOK_KEYWORD_FN:
            return parse_fn(p);

/*
        case TOK_OPEN_BRACKET:
            if (peek_token(p, 1).type != TOK_CLOSE_BRACKET) {
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
*/

        case TOK_OPEN_BRACKET:
            //("^" | "[]") ("mut" | "let") ( <type> | E)
            if (peek_token(p, 1).type != TOK_CLOSE_BRACKET) {
                advance_token(p);
                n = new_ast_node(p, AST_array_type_expr);
                n.as_array_type_expr->base.start = &current_token(p);
                n.as_array_type_expr->length = parse_expr(p);
                if (current_token(p).type != TOK_CLOSE_BRACKET) error_at_parser(p, "expected ]");
                advance_token(p);
                n.as_array_type_expr->type = parse_type(p);
                n.as_array_type_expr->base.end = &current_token(p);
                return n;
            }
            advance_token(p);
            // if (current_token(p).type != TOK_CLOSE_BRACKET) error_at_parser(p, "expected ]");
            n = new_ast_node(p, AST_slice_type_expr);
        case TOK_CARET:
            if (is_null_AST(n)) n = new_ast_node(p, AST_pointer_type_expr);
            n.as_slice_type_expr->base.start = &current_token(p);
            advance_token(p);
            if (current_token(p).type != TOK_KEYWORD_MUT && current_token(p).type != TOK_KEYWORD_LET) error_at_parser(p, "expected mut or let");
            n.as_pointer_type_expr->mutable = current_token(p).type == TOK_KEYWORD_MUT;
            advance_token(p);
            n.as_pointer_type_expr->subexpr = parse_type(p);
            n.as_pointer_type_expr->base.end = &current_token(p);
            return n;
        //"distinct" <unary_expression>
        case TOK_KEYWORD_DISTINCT:
            n = new_ast_node(p, AST_distinct_type_expr);
            n.as_distinct_type_expr->base.start = &current_token(p);
            advance_token(p);
            n.as_distinct_type_expr->subexpr = parse_type(p);
            n.as_distinct_type_expr->base.end = &current_token(p);
            return n;
    }
    return NULL_AST;
}

AST parse_atomic_expr(parser* p) {
    debug_trace(p);
    //this function handles explicitly atomic_expr that require left-assoc stuff.

/*<atomic_expression> ::= <literal> |  "." <identifier> | 
                        "(" <expression> ")" | <fn_literal> | <type> |
                        <type> "{" ((<expression> ",")* <expression> ("," | E) | E) "}" |
                        <atomic_expression> ("::" | "." | "->") <identifier> |
                        <atomic_expression> "[" <expression> "]" |
                        <atomic_expression> "[" (<expression> | E) ":" (<expression> | E) "]" |
                        <atomic_expression> "(" ((<expression> ",")* <expression> ("," | E) | E) ")" |
                        <atomic_expression> "^" */

    AST left = parse_atomic_expr_term(p);
    AST left_copy = NULL_AST;

    if (is_null_AST(left)) return left;

    while (current_token(p).type != TOK_EOF) {
        if (current_token(p).type == TOK_COLON_COLON || current_token(p).type == TOK_PERIOD || current_token(p).type == TOK_ARROW_RIGHT) {
            //<atomic_expression> ("::" | "." | "->") <identifier>
            if (current_token(p).type == TOK_PERIOD && peek_token(p, 1).type == TOK_OPEN_BRACE) {
                // compound literal of form type.{1, 2, 3}
                left_copy = left;
                left = new_ast_node(p, AST_comp_literal_expr);
                left.as_comp_literal_expr->type = left_copy;
                left.as_comp_literal_expr->base.start = left_copy.base->start;
                da_init(&left.as_comp_literal_expr->elems, 4);
                advance_token(p);
                advance_token(p);
                while (current_token(p).type != TOK_CLOSE_BRACE) {
                    AST elem = parse_expr(p);
                    da_append(&left.as_comp_literal_expr->elems, elem);
                    if (current_token(p).type == TOK_COMMA) {
                        advance_token(p);
                        continue;
                    } else if (current_token(p).type != TOK_CLOSE_BRACE) {
                        error_at_parser(p, "expected , or }");
                    }
                }
                left.as_comp_literal_expr->base.end = &current_token(p);
                advance_token(p);
                continue;
            }
            left_copy = left;
            left = new_ast_node(p, AST_selector_expr);
            left.as_selector_expr->base.start = left_copy.base->start;
            left.as_selector_expr->lhs = left_copy;
            left.as_selector_expr->op = &current_token(p);
            advance_token(p);
            left.as_selector_expr->rhs = parse_identifier(p);
            left.as_selector_expr->base.end = &peek_token(p, -1);
            continue;
        }
        //<atomic_expression> "[" <expression> "]" |
        //<atomic_expression> "[" (<expression> | E) ":" (<expression> | E) "]" |
        if (current_token(p).type == TOK_OPEN_BRACKET) {
            token* start = &current_token(p);
            advance_token(p);
            AST expr = parse_expr(p);
            if (is_null_AST(expr) && current_token(p).type != TOK_CLOSE_BRACKET) error_at_parser(p, "expected : or ]");
            //<atomic_expression> "[" <expression> "]"
            if (current_token(p).type == TOK_CLOSE_BRACKET) {
                left_copy = left;
                left = new_ast_node(p, AST_index_expr);
                left.as_index_expr->base.start = left_copy.base->start;
                left.as_index_expr->lhs = left_copy;
                left.as_index_expr->inside = expr;
                left.as_index_expr->base.end = &current_token(p);
                advance_token(p);
                continue;
            }
            //<atomic_expression> "[" (<expression> | E) ":" (<expression> | E) "]"
            left_copy = left;
            left = new_ast_node(p, AST_slice_expr);
            left.as_slice_expr->base.start = left_copy.base->start;
            left.as_slice_expr->lhs = left_copy;
            left.as_slice_expr->inside_left = expr;
            if (current_token(p).type != TOK_COLON) error_at_parser(p, "expected :");
            advance_token(p);
            left.as_slice_expr->inside_right = parse_expr(p);
            if (current_token(p).type != TOK_CLOSE_BRACKET) error_at_parser(p, "expected ]");
            advance_token(p);
            continue;
        }
        //<atomic_expression> "(" ((<expression> ",")* <expression> | E) ")"
        if (current_token(p).type == TOK_OPEN_PAREN) {
            left_copy = left;
            left = new_ast_node(p, AST_call_expr);
            left.as_call_expr->lhs = left_copy;
            left.as_call_expr->base.start = left_copy.base->start;
            da_init(&left.as_call_expr->params, 1);
            advance_token(p);
            if (current_token(p).type == TOK_CLOSE_PAREN) {
                left.as_call_expr->base.end = &current_token(p);
                continue;
            }
            while (current_token(p).type != TOK_CLOSE_PAREN) {
                da_append(&left.as_call_expr->params, parse_expr(p));
                if (current_token(p).type == TOK_COMMA) advance_token(p);
            }
            left.as_call_expr->base.end = &current_token(p);
            advance_token(p);
            continue;
        }
        //<atomic_expression> "^"
        if (current_token(p).type == TOK_CARET) {
            left_copy = left;
            left = new_ast_node(p, AST_unary_op_expr);
            left.as_unary_op_expr->op = &current_token(p);
            left.as_unary_op_expr->inside = left_copy;
            left.as_unary_op_expr->base.start = left_copy.base->start;
            left.as_unary_op_expr->base.end = &current_token(p);
            advance_token(p);
            continue;
        } 
        return left;
    }
    return left;
}

AST parse_stmt_block(parser* p) {
    debug_trace(p);
    //<stmt_block> ::=  "{" <stmt>* "}"
    AST n = new_ast_node(p, AST_stmt_block); 
    da_init(&n.as_stmt_block->stmts, 1);

    if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");

    advance_token(p);
    while (current_token(p).type != TOK_CLOSE_BRACE) {
        AST stmt = parse_stmt(p);
        if (is_null_AST(stmt)) break;
        da_append(&n.as_stmt_block->stmts, stmt);
    }
    advance_token(p);
    return n;
}

AST parse_atomic_expr_term(parser* p) {
    debug_trace(p);
    //this function is for <atomic_expression> bnf leafs with terminals explicitly in them
    //not left-assoc, basically
    /*<atomic_expression_terminals> ::= <type> | <literal> | <identifier> | "." <identifier> | 
                                        <fn_literal> | "(" <expression> ")" | 
                                        <type> "{" ((<expression> ",")* <expression> ("," | E) | E) "}" |*/

    AST n;

    n = parse_type(p);
    // sorry wait this is 
    // breaking my brain 
    // wait
    // ok so i can just remove this right
    // and put it down into the . shit by the enums
    if (!is_null_AST(n)) {
        return n;
    }
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
        // "." <identifier> | "." <compound_lit>
        case TOK_PERIOD:
            if (peek_token(p, 1).type == TOK_IDENTIFIER) {
                n = new_ast_node(p, AST_selector_expr);
                n.as_selector_expr->base.start = &current_token(p);
                n.as_selector_expr->op = &current_token(p);
                advance_token(p);
                n.as_selector_expr->lhs = NULL_AST;
                n.as_selector_expr->rhs = parse_identifier(p);
                n.as_identifier->base.end = &current_token(p);
                advance_token(p);
            } else if (peek_token(p, 1).type == TOK_OPEN_BRACE) {
                AST lit = new_ast_node(p, AST_comp_literal_expr);
                lit.as_comp_literal_expr->type = n;
                lit.as_comp_literal_expr->base.start = &current_token(p);
                da_init(&lit.as_comp_literal_expr->elems, 4);
                advance_token(p);
                advance_token(p);
                if (current_token(p).type == TOK_CLOSE_BRACE) {
                    lit.as_comp_literal_expr->base.end = &current_token(p);
                    return lit;
                }
                while (current_token(p).type != TOK_CLOSE_BRACE) {
                    da_append(&lit.as_comp_literal_expr->elems, parse_expr(p));
                    if (current_token(p).type == TOK_COMMA) advance_token(p);
                }
                lit.as_comp_literal_expr->base.end = &current_token(p);
                advance_token(p);
                return lit;
            } else {
                advance_token(p);
                error_at_parser(p, "expected identifier or {");
            }
            return n;
        //fn lit or fn ptr. who knows?
        case TOK_KEYWORD_FN:
            return parse_fn(p);
        case TOK_OPEN_PAREN:
            advance_token(p);
            n = parse_expr(p);
            if (current_token(p).type != TOK_CLOSE_PAREN) error_at_parser(p, "expected )");
            advance_token(p);
            return n;
    }
    return NULL_AST;
}

AST parse_aggregate(parser* p) {
    debug_trace(p);
    // <aggregate> ::= ("struct" | "union")  "{" <param_list> "}" 
    AST n = new_ast_node(p, AST_struct_type_expr);
    n.as_struct_type_expr->base.start = &current_token(p);
    n.as_struct_type_expr->is_union = current_token(p).type == TOK_KEYWORD_UNION;
    da_init(&n.as_struct_type_expr->fields, 1);
    advance_token(p);
    if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");
    advance_token(p);
    if (current_token(p).type != TOK_CLOSE_BRACE) { 
        /*        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>

        */
        for (int i = p->current_tok; i < p->tokens.len; i++) {
            if (current_token(p).type == TOK_COLON) error_at_parser(p, "unexpected :");
            if (current_token(p).type == TOK_COMMA) advance_token(p);
            if (current_token(p).type == TOK_CLOSE_BRACE) break;
            if (current_token(p).type == TOK_IDENTIFIER) {
                if (peek_token(p, 1).type == TOK_COLON) {
                    //<param> ::= <identifier> ":" <type>
                    AST_typed_field curr_field = {0};
                    AST field = parse_identifier(p);
                    advance_token(p);
                    da_append(&n.as_struct_type_expr->fields, ((AST_typed_field){.field = field, .type = parse_type(p)}));
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA) { 
                    //<multi_param> ::= (<identifier>  ",")+ <identifier> ":" <type> 
                    AST_typed_field curr_field;
                    curr_field.field = parse_identifier(p);
                    curr_field.type.type = 0;
                    da_append(&n.as_struct_type_expr->fields, curr_field);
                    advance_token(p);
                    continue;
                }
            }
            error_at_parser(p, "unexpected");
        }
    }
    n.as_struct_type_expr->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_enum(parser* p) {
    debug_trace(p);
    //enum's param_list separator is =, not :
    //<enum> ::= "enum" (<type> | E) "{" <param_list> "}"
    AST n = new_ast_node(p, AST_enum_type_expr);
    n.as_enum_type_expr->base.start = &current_token(p);
    n.as_enum_type_expr->backing_type = parse_type(p);
    da_init(&n.as_enum_type_expr->variants, 1);
    advance_token(p);
    if (current_token(p).type != TOK_OPEN_BRACE) error_at_parser(p, "expected {");
    advance_token(p);
    if (current_token(p).type != TOK_CLOSE_BRACE) { 
        /*        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>    */
        for (int i = p->current_tok; i < p->tokens.len; i++) {
            if (current_token(p).type == TOK_EQUAL) error_at_parser(p, "unexpected :");
            if (current_token(p).type == TOK_COMMA) advance_token(p);
            if (current_token(p).type == TOK_CLOSE_BRACE) break;
            if (current_token(p).type == TOK_IDENTIFIER) {
                if (peek_token(p, 1).type == TOK_EQUAL) {
                    //<param> ::= <identifier> "=" <expression>
                    AST_enum_variant var = {0};
                    var.ident = parse_identifier(p);
                    advance_token(p); advance_token(p);
                    var.expr = parse_expr(p);
                    var.tok = &current_token(p);
                    da_append(&n.as_enum_type_expr->variants, var);
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA) { 
                    //<multi_param> ::= (<identifier> ",")+ <identifier> "=" <expression> 
                    AST_enum_variant var = {0};
                    var.ident = parse_identifier(p);
                    advance_token(p); advance_token(p);
                    var.expr = NULL_AST;
                    var.tok = NULL;
                    da_append(&n.as_enum_type_expr->variants, var);
                    continue;
                }
            }
            error_at_parser(p, "unexpected");
        }
    }
    n.as_enum_type_expr->base.end = &current_token(p);
    advance_token(p);
    return n;
}

int verify_type(parser* p) {
    switch (peek_token(p, 1).type) {
        case TOK_IDENTIFIER:
        case TOK_KEYWORD_FN:
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
        case TOK_KEYWORD_STRUCT:
        case TOK_KEYWORD_UNION:
            return 1;
        default:
            return 0;
    }
}

AST parse_fn(parser* p) {
    debug_trace(p);
    /*<fn_literal> ::= "fn" "(" (<param_list> ("," | E) | E) ")" ("->" (<type> | "(" <param_list> ")") | E) <stmt_block>

        <param> ::= <identifier> ":" <type> 
        <multi_param> ::= (<identifier>  ",")+ <identifier> ":" <type> 
        <param_list> ::= <param_list> "," <param> | 
                         <param_list> "," <multi_param> | 
                         <param> | <multi_param>
        */
    //OR:
    //<fn_pointer> ::= "fn" "(" ((<type> ",")* <type> | E) ")" ("->" (<type> | "(" (<type> ",")* <type> ")")| E) 

    AST n = new_ast_node(p, AST_fn_type_expr);
    n.as_fn_type_expr->base.start = &current_token(p);
    da_init(&n.as_fn_type_expr->parameters, 1);
    da_init(&n.as_fn_type_expr->returns, 1);
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
                    n.as_fn_type_expr->is_literal = true;
                    //<param> ::= <unary_expression> ":" <unary_expression>
                    AST_typed_field curr_field = {0};
                    AST field = parse_unary_expr(p);
                    advance_token(p);
                    da_append(&n.as_fn_type_expr->parameters, ((AST_typed_field){.field = field, .type = parse_type(p)}));
                    continue;
                }
                if (peek_token(p, 1).type == TOK_COMMA || peek_token(p, 1).type == TOK_CLOSE_PAREN) { 
                    //<multi_param> ::= (<unary_expression>  ",")+ <unary_expression> ":" <unary_expression> 
                    AST_typed_field curr_field;
                    curr_field.field = parse_unary_expr(p);
                    curr_field.type.type = 0;
                    da_append(&n.as_fn_type_expr->parameters, curr_field);
                    if (current_token(p).type != TOK_CLOSE_PAREN) advance_token(p);
                    continue;
                }
                error_at_parser(p, "expected , after identifier");
            }
        }
    }
    if (current_token(p).type != TOK_CLOSE_PAREN) error_at_parser(p, "expected )");
    advance_token(p);

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
                        n.as_fn_type_expr->is_literal = true;
                        //<param> ::= <identifier> ":" <type>
                        AST_typed_field curr_field = {0};
                        AST field = parse_identifier(p);
                        advance_token(p);
                        da_append(&n.as_fn_type_expr->returns, ((AST_typed_field){.field = field, .type = parse_type(p)}));
                        continue;
                    }
                    if (peek_token(p, 1).type == TOK_COMMA || peek_token(p, 1).type == TOK_CLOSE_PAREN) { 
                        //<multi_param> ::= (<identifier>  ",")+ <identifier> ":" <type> 
                        AST_typed_field curr_field;
                        curr_field.field = parse_identifier(p);
                        curr_field.type.type = 0;
                        da_append(&n.as_fn_type_expr->returns, curr_field);
                        if (current_token(p).type != TOK_CLOSE_PAREN) advance_token(p);
                        continue;
                    }
                }
                error_at_parser(p, "INTERNAL COMPILER ERROR: fell out of fn_type parse loop for returns");
            }
            advance_token(p);
        } else {
            AST type = parse_type(p);
            if (is_null_AST(type)) error_at_parser(p, "expected type or identifier");
            AST_typed_field curr_field = {0};
            curr_field.type = type;
            curr_field.field = type;
            da_append(&n.as_fn_type_expr->returns, curr_field);
        }
    }
    n.as_fn_type_expr->base.end = &current_token(p);

    if (n.as_fn_type_expr->is_literal == true) {
        AST lit = new_ast_node(p, AST_func_literal_expr);
        lit.as_func_literal_expr->base.start = n.as_fn_type_expr->base.start;

        lit.as_func_literal_expr->type = n;
        lit.as_func_literal_expr->code_block = parse_stmt_block(p);
        lit.as_func_literal_expr->base.end = &current_token(p);

        return lit;        
    }

    //we now need to swap all AST_typed_field info, since its in the wrong place entirely.
    for (int i = 0; i < n.as_fn_type_expr->parameters.len; i++) {
        n.as_fn_type_expr->parameters.at[i].type = n.as_fn_type_expr->parameters.at[i].field;
        n.as_fn_type_expr->parameters.at[i].field = NULL_AST;
    }

    for (int i = 0; i < n.as_fn_type_expr->returns.len; i++) {
        n.as_fn_type_expr->returns.at[i].type = n.as_fn_type_expr->returns.at[i].field;
        n.as_fn_type_expr->returns.at[i].field = NULL_AST;
    }

    return n;
}

AST parse_unary_expr(parser* p) {
    debug_trace(p);
    /*<unary_expression> ::= <unop> <unary_expression> |
                       ("cast" | "bitcast") "(" <unary_expression> ")" <unary_expression> |
                       ("^" | "[]") ("mut" | "let") ( <unary_expression> | E) |
                       "[" <expression> "]" <unary_expression> | 
                       "distinct" <unary_expression> |
                       <atomic_expression>*/
    AST n = NULL_AST;

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
        default:
            return parse_atomic_expr(p);
    }
}

AST parse_expr(parser* p) {
    debug_trace(p);
    //this is gonna be painful
    //we first need to detect WHICH expr branch we're on.
    //
    /*<expression> ::= <expression> <binop> <expression> | <unary_expression>*/

    if (current_token(p).type == TOK_OPEN_BRACE) error_at_parser(p, "unexpected {");

    //<unary_expression>
    return parse_binop_expr(p, -1);
}

AST parse_binop_expr(parser* p, int precedence) {
    debug_trace(p);
    //pratt parsing!
    //explanation: we keep tr
    //ack of a precedence p when we start.
    //we then parse, and if this precedence has changed, we break out of this loop 
    AST lhs = parse_unary_expr(p);
    if (is_null_AST(lhs)) return lhs;
    
    while (precedence < verify_binop(p, current_token(p), false)) {
        lhs = parse_binop_recurse(p, lhs, precedence);
    }

    return lhs;
}

AST parse_binop_recurse(parser* p, AST lhs, int precedence) {
    debug_trace(p);
    AST n = new_ast_node(p, AST_binary_op_expr);
    n.as_binary_op_expr->base.start = lhs.base->start;
    n.as_binary_op_expr->op = &current_token(p);
    n.as_binary_op_expr->lhs = lhs;
    //current_token is a binop here
    advance_token(p);

    n.as_binary_op_expr->rhs = parse_binop_expr(p, verify_binop(p, *n.as_binary_op_expr->op, true));
    if (is_null_AST(n.as_binary_op_expr->rhs)) error_at_parser(p, "expected operand");
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
    debug_trace(p);
    //<decl> ::= ("let" | "mut") (<identifier> ",")* <identifier> ("," | E) 
    //(":" <type> | E) "=" <r_value> ";"
    AST n = new_ast_node(p, AST_decl_stmt);
    n.as_decl_stmt->base.start = &current_token(p);
    da_init(&n.as_decl_stmt->lhs, 1);
    if (current_token(p).type == TOK_KEYWORD_MUT) n.as_decl_stmt->is_mut = 1;
    advance_token(p);
    
    if (current_token(p).type != TOK_IDENTIFIER) error_at_parser(p, "expected identifier when parsing lhs");
    while (current_token(p).type == TOK_IDENTIFIER) {
        AST ident = parse_identifier(p);
        da_append(&n.as_decl_stmt->lhs, ident);
        if (current_token(p).type == TOK_COMMA) advance_token(p);
    } 

    if (current_token(p).type == TOK_COLON) {
        advance_token(p);
        if (current_token(p).type == TOK_EQUAL) error_at_parser(p, "expected type");
        n.as_decl_stmt->type = parse_type(p);
    }
    if (current_token(p).type != TOK_EQUAL && current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected either = or ;");
    advance_token(p);
    if (current_token(p).type == TOK_UNINIT) n.as_decl_stmt->rhs = NULL_AST;
    else {n.as_decl_stmt->rhs = parse_expr(p);}
    if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected ;");
    n.as_decl_stmt->base.end = &current_token(p);
    advance_token(p);
    return n;
}

AST parse_module_decl(parser* p) {
    debug_trace(p);
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
    debug_trace(p);
    //<import_stmt> ::= "import" (<identifier> | E) <string> ";" 
    AST n = new_ast_node(p, AST_import_stmt);
    n.as_import_stmt->base.start = &current_token(p);
    advance_token(p);
    if (current_token(p).type == TOK_IDENTIFIER) {
        n.as_import_stmt->name = parse_identifier(p);

    }

    if (current_token(p).type != TOK_LITERAL_STRING) error_at_parser(p, "expected string literal");
    n.as_import_stmt->path = new_ast_node(p, AST_literal_expr);
    n.as_import_stmt->path.as_literal_expr->tok = &current_token(p);
    advance_token(p);
    if (current_token(p).type != TOK_SEMICOLON) error_at_parser(p, "expected semicolon");
    n.as_import_stmt->base.end = &current_token(p);
    advance_token(p);
    return n;
}