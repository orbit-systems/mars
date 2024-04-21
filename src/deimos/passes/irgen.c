#include "deimos.h"
#include "passes.h"
#include "phobos/sema.h"

static mars_module* mars_mod;

IR_Module* ir_pass_generate(mars_module* mod) {
	IR_Module* m = ir_new_module(mod->module_name);
    mars_mod = mod;
	
	/* do some codegen shit prolly */

	return m;
}

IR* ir_generate_expr_literal(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_literal_expr* literal = ast.as_literal_expr;
    IR_Const* ir = (IR_Const*) ir_make_const(f);
    
    switch (literal->value.kind) {
    case EV_UNTYPED_INT:
    case EV_I64:
        ir->base.T = make_type(TYPE_I64);
        ir->i64 = literal->value.as_i64;
    case EV_U64:
        ir->base.T = make_type(TYPE_U64);
        ir->u64 = literal->value.as_u64;
    default:
        warning_at_node(mars_mod, ast, "unhandled EV type");
        CRASH("unhandled EV type");
    }

    ir_add(bb, (IR*) ir);
    return (IR*) ir;
}

IR* ir_generate_expr_binop(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_binary_op_expr* binop = ast.as_binary_op_expr;

    IR* lhs = ir_generate_expr(f, bb, binop->lhs);
    IR* rhs = ir_generate_expr(f, bb, binop->rhs);
    IR_BinOp* ir = (IR_BinOp*) ir_make_binop(f, IR_INVALID, lhs, rhs);

    switch (binop->op->type) {
    case TOK_ADD: ir->base.tag = IR_ADD; break;
    case TOK_SUB: ir->base.tag = IR_SUB; break;
    case TOK_MUL: ir->base.tag = IR_MUL; break;
    case TOK_DIV: ir->base.tag = IR_DIV; break;
    default:
        warning_at_node(mars_mod, ast, "unhandled binop type");
        CRASH("unhandled binop type");
        break;
    }

    return;
}

typedef struct EntityExtra {
    IR* stackalloc;
} EntityExtra;

// this generates a load btw!!!
IR* ir_generate_expr_ident_load(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_identifier_expr* ident = ast.as_identifier_expr;
    if (ident->is_discard) return ir_add(bb, ir_make(f, IR_INVALID));

    // if a stackalloc and entityextra havent been generated, generate them
    if (!ident->entity->extra) {
        IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, 
            type_real_size_of(ident->entity->entity_type),
            type_real_align_of(ident->entity->entity_type),
            ident->entity->entity_type
        ));

        EntityExtra* extra = arena_alloc(&f->alloca, sizeof(*extra), alignof(*extra));
        extra->stackalloc = stackalloc;

        ident->entity->extra = extra;
    }
    IR* stackalloc = ((EntityExtra*)ident->entity->extra)->stackalloc;

    return ir_add(bb, ir_make_load(f, stackalloc, false));
}

IR* ir_generate_expr(IR_Function* f, IR_BasicBlock* bb, AST ast) {

    switch (ast.type) {
    case AST_literal_expr:    return ir_generate_expr_literal(f, bb, ast);
    case AST_binary_op_expr:  return ir_generate_expr_binop(f, bb, ast);
    case AST_identifier_expr: return ir_generate_expr_ident_load(f, bb, ast);

    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
        break;
    }
}