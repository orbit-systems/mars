#include "deimos.h"
#include "passes.h"
#include "phobos/sema.h"

static mars_module* mars_mod;

IR* ir_generate_expr_value(IR_Function* f, IR_BasicBlock* bb, AST ast);

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

    IR* lhs = ir_generate_expr_value(f, bb, binop->lhs);
    IR* rhs = ir_generate_expr_value(f, bb, binop->rhs);
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

    return ir;
}

typedef struct EntityExtra {
    IR* stackalloc;
} EntityExtra;

IR* ir_generate_expr_ident_load(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    ast_identifier_expr* ident = ast.as_identifier_expr;
    if (ident->is_discard) return ir_add(bb, ir_make(f, IR_INVALID));

    // if a stackalloc and entityextra havent been generated, generate them
    if (!ident->entity->extra) {

        if (!ident->entity->entity_type) {
            warning_at_node(mars_mod, ast, "bodge! assuming i64 type");
            ident->entity->entity_type = make_type(TYPE_I64);
        }

        IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, ident->entity->entity_type));

        EntityExtra* extra = arena_alloc(&f->alloca, sizeof(*extra), alignof(*extra));
        extra->stackalloc = stackalloc;

        ident->entity->extra = extra;
    }
    IR* stackalloc = ((EntityExtra*)ident->entity->extra)->stackalloc;

    return ir_add(bb, ir_make_load(f, stackalloc, false));
}

IR* ir_generate_expr_value(IR_Function* f, IR_BasicBlock* bb, AST ast) {

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

IR* ir_generate_expr_address(IR_Function* f, IR_BasicBlock* bb, AST ast) {
    switch (ast.type) {
    case AST_identifier_expr:
        if (ast.as_identifier_expr->entity->extra != NULL) {
            return ((EntityExtra*)ast.as_identifier_expr->entity->extra)->stackalloc;
        }
        ast_identifier_expr* ident = ast.as_identifier_expr;
        if (!ident->entity->extra) {

            // generate stackalloc
            IR* stackalloc = ir_add(bb, ir_make_stackalloc(f, ident->entity->entity_type));
            TODO("bruh");
        }
        break;
    default:
        warning_at_node(mars_mod, ast, "unhandled AST type");
        CRASH("unhandled AST type");
    }
}

IR* ir_generate_stmt_assign(IR_Function*f, IR_BasicBlock* bb, AST ast) {
    ast_assign_stmt* assign = ast.as_assign_stmt;

    if (assign->lhs.len == 1) {
        IR* dest_address = ir_generate_expr_address(f, bb, assign->lhs.at[0]);

        if (assign->rhs.type == AST_call_expr) {
            // function calls need special handling
            warning_at_node(mars_mod, ast, "unhandled call in assignment");
            CRASH("unhandled call in assignment");
        }

        // this does not take into account struct assignments either, just regular ol values

        if (!(assign->rhs.type == AST_identifier_expr && assign->rhs.as_identifier_expr->is_discard)) {
            IR* source_val = ir_generate_expr_value(f, bb, assign->rhs);
            IR* store = ir_add(bb, ir_make_store(f, dest_address, source_val, false));
        }
    } else {
        warning_at_node(mars_mod, ast, "unhandled multi-assign");
        CRASH("unhandled multi-assign");
    }
}

IR_Function* ir_generate_function(IR_Module* mod, AST ast) {
    if (ast.type != AST_func_literal_expr) CRASH("ast type is not func literal");
    ast_func_literal_expr* astfunc = ast.as_func_literal_expr;

    // assume all functions are global at the moment
    IR_Function* f = ir_new_function(mod, NULL, true);
    IR_BasicBlock* bb = ir_new_basic_block(f, str("begin"));

    da(AST_typed_field) params = astfunc->type.as_fn_type_expr->parameters;

    // generate entity storage for parameters
    FOR_URANGE(i, 0, params.len) {
        if (params.at->field.base->T != TYPE_I64) {
            CRASH("only i64 params supported (for testing)");
        }
        IR* paramval = ir_add(bb, ir_make_paramval(f, i));

    }

    return f;
}