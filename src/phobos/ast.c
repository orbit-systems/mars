#include "orbit.h"
#include "lexer.h"
#include "error.h"
#include "ast.h"
#include "arena.h"

size_t ast_type_size[] = {
    0,
#define AST_TYPE(ident, identstr, structdef) sizeof(ast_##ident),
    AST_NODES
#undef AST_TYPE
    0
};

char* ast_type_str[] = {
    "invalid",
#define AST_TYPE(ident, identstr, structdef) identstr,
    AST_NODES
#undef AST_TYPE
    "COUNT",
};

// allocate and zero a new AST node with an arena
AST new_ast_node(arena* restrict a, ast_type type) {
    AST node;
    void* node_ptr = arena_alloc(a, ast_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_ast_node() could not allocate AST node of type '%s' with size %d", ast_type_str[type], ast_type_size[type]);
    }
    memset(node_ptr, 0, ast_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    return node;
}

void print_indent(int n) {
    FOR_RANGE_EXCL(i, 0, n) printf("|    ");
}

// FOR DEBUGGING PURPOSES!! THIS IS NOT GOING TO BE MEMORY SAFE LMFAO
void dump_tree(AST node, int n) {

    if (is_null_AST(node)) return;

    print_indent(n);

    switch (node.type) {
    case astype_invalid:
        printf("[INVALID]\n");
        break;
    case astype_identifier_expr:
        printf("identifier '%s'\n", clone_to_cstring(node.as_identifier_expr->tok->text));
        break;
    case astype_literal_expr:
        switch (node.as_literal_expr->value.kind) {
        case ev_bool: printf("bool literal\n"); break;
        case ev_float: printf("bool literal\n"); break;
        case ev_int: printf("int literal\n"); break;
        case ev_pointer: printf("pointer literal\n"); break;
        case ev_string: printf("string literal\n"); break;
        default: printf("[invalid] literal\n"); break;
        }
        break;
    case astype_paren_expr:
        printf("()\n");
        dump_tree(node.as_paren_expr->subexpr, n+1);
        break;
    case astype_cast_expr:
        if (node.as_cast_expr->is_bitcast) printf("bitcast\n");
        else printf("cast\n");
        dump_tree(node.as_cast_expr->type, n+1);
        dump_tree(node.as_cast_expr->rhs, n+1);
        break;
    case astype_unary_op_expr:
        printf("unary %s\n", token_type_str[node.as_unary_op_expr->op->type]);
        dump_tree(node.as_unary_op_expr->inside, n+1);
        break;
    case astype_binary_op_expr:
        printf("binary %s\n", token_type_str[node.as_binary_op_expr->op->type]);
        dump_tree(node.as_binary_op_expr->lhs, n+1);
        dump_tree(node.as_binary_op_expr->rhs, n+1);
        break;
    case astype_entity_selector_expr:
        printf("entity selector\n");
        dump_tree(node.as_entity_selector_expr->lhs, n+1);
        dump_tree(node.as_entity_selector_expr->rhs, n+1);
        break;
    case astype_selector_expr:
        printf("selector\n");
        dump_tree(node.as_selector_expr->lhs, n+1);
        dump_tree(node.as_selector_expr->rhs, n+1);
        break;
    case astype_impl_selector_expr:
        printf("implicit selector\n");
        dump_tree(node.as_impl_selector_expr->rhs, n+1);
        break;
    case astype_index_expr:
        printf("index\n");
        dump_tree(node.as_index_expr->lhs, n+1);
        dump_tree(node.as_index_expr->inside, n+1);
        break;
    case astype_slice_expr:
        printf("slice\n");
        dump_tree(node.as_slice_expr->lhs, n+1);
        dump_tree(node.as_slice_expr->inside_left, n+1);
        dump_tree(node.as_slice_expr->inside_right, n+1);
        break;
    case astype_call_expr:
        printf("function call\n");
        dump_tree(node.as_call_expr->lhs, n+1);
        FOR_URANGE_EXCL(i, 0, node.as_call_expr->params.len) {
            dump_tree(node.as_call_expr->params.raw[i], n+1);
        }
        break;
    
    case astype_module_decl:
        printf("module %s\n", clone_to_cstring(node.as_module_decl->name->text));
        break;
    case astype_import_stmt:
        printf("import\n");
        dump_tree(node.as_import_stmt->name, n+1);
        dump_tree(node.as_import_stmt->path, n+1);
        break;
    case astype_block_stmt:
        printf("{;}\n");
        FOR_URANGE_EXCL(i, 0, node.as_block_stmt->stmts.len) {
            dump_tree(node.as_block_stmt->stmts.raw[i], n+1);
        }
        break;
    case astype_decl_stmt:
        if (node.as_decl_stmt->is_mut) 
            printf("mut \n");
        else 
            printf("let \n");

        FOR_URANGE_EXCL(i, 0, node.as_decl_stmt->lhs.len) {
            dump_tree(node.as_decl_stmt->lhs.raw[i], n+1);
        }
        dump_tree(node.as_decl_stmt->type, n+1);
        dump_tree(node.as_decl_stmt->rhs, n+1);

        break;
    default:
        general_error("dump_tree(): invalid/implemented node of type %d '%s'", node.type, ast_type_str[node.type]);
        break;
    }
}