#include "irgen.h"
#include "lex.h"
#include "parse.h"
#include "common/strmap.h"

// typedef struct IrGenFunction {
//     Entity* entity;
//     FeSymbol* symbol;
// } IrGenFunction;

FeModule* irgen(CompilationUnit* cu) {

    IRGen* ig = malloc(sizeof(IRGen));

    ig->cu = cu;

    ig->m = fe_module_new(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    FeInstPool* ipool = malloc(sizeof(FeInstPool));
    fe_ipool_init(ipool);
    FeVRegBuffer* vregs = malloc(sizeof(FeVRegBuffer));
    fe_vrbuf_init(vregs, 2048);

    ig->default_text = fe_section_new(ig->m, "text", 4, FE_SECTION_EXECUTABLE);
    ig->default_data = fe_section_new(ig->m, "data", 4, FE_SECTION_WRITEABLE);
    ig->default_rodata = fe_section_new(ig->m, "rodata", 6, 0);

    // printf("\nFUNCTIONS:\n");
    // for_n(i, 0, cu->top_scope->map.cap) {
    //     Entity* e = cu->top_scope->map.vals[i];
    //     if (e == nullptr) {
    //         continue;
    //     }
    //     if (e->kind != ENTKIND_FN) {
    //         continue;
    //     }
    //     printf("    "str_fmt"\n", str_arg(from_compact(e->name)));
    // }

    // printf("\nSTATIC VARIABLES:\n");
    // for_n(i, 0, cu->top_scope->map.cap) {
    //     Entity* e = cu->top_scope->map.vals[i];
    //     if (e == nullptr) {
    //         continue;
    //     }
    //     if (e->kind != ENTKIND_VAR) {
    //         continue;
    //     }
    //     printf("    "str_fmt"\n", str_arg(from_compact(e->name)));
    // }

    // printf("\nTYPES:\n");
    // for_n(i, 0, cu->top_scope->map.cap) {
    //     Entity* e = cu->top_scope->map.vals[i];
    //     if (e == nullptr) {
    //         continue;
    //     }
    //     if (e->kind != ENTKIND_TYPE) {
    //         continue;
    //     }
    //     printf("    "str_fmt"\n", str_arg(from_compact(e->name)));
    // }

    // printf("\n%d symbols\n%d capacity\n", cu->top_scope->map.size, cu->top_scope->map.cap);

    // pre-generate symbols for top-level entities    
    for_n(i, 0, cu->top_scope->map.cap) {
        Entity* e = cu->top_scope->map.vals[i];
        if (e == nullptr) {
            continue;
        }
        // generate for functions first
        if (e->kind != ENTKIND_FN) {
            continue;
        }

        FeSymbolBinding bind;
        switch (e->storage) {
        case STORAGE_PUBLIC:
            bind = FE_BIND_GLOBAL;
            break;
        case STORAGE_PRIVATE:
            bind = FE_BIND_LOCAL;
            break;
        case STORAGE_EXTERN:
            bind = FE_BIND_EXTERN;
            break;
        case STORAGE_EXPORT:
        case STORAGE_LOCAL:
        case STORAGE_OUT_PARAM:
            UNREACHABLE;
        }

        // FeSection* section = e->kind == ENTKIND_FN ? default_text : default_data;
        FeSection* section = ig->default_text;
        FeSymbol* sym = fe_symbol_new(ig->m, from_compact(e->name).raw, e->name.len, section, bind);
    
        FeFuncSig* signature = generate_signature(cu, ig->m, e);

        FeFunc* func = fe_func_new(ig->m, sym, signature, ipool, vregs);

        ig->f = func;
        ig->e = e;
        ig->b = func->entry_block;

        irgen_function(ig);
    }


    return ig->m;
}

#define TY(index, T) ((T*)&tybuf.at[index])
#define TY_KIND(index) ((TyBase*)&tybuf.at[index])->kind

static FeInst* irgen_expr_literal(IRGen* ig, Expr* expr) {
    assert(expr->ty <= TY_UQUAD);

    TySelectResult iron_ty = select_iron_type(expr->ty);
    FeInst* lit = fe_append_end(ig->b, fe_inst_const(ig->f, iron_ty.ty, expr->literal));
    return lit;
}

static FeInst* irgen_expr_entity(IRGen* ig, Expr* expr) {
    switch (expr->entity->storage) {
    case STORAGE_LOCAL: {
        expr->entity->fe_ty = select_iron_type(expr->ty).ty;

        FeInst* load = fe_append_end(ig->b, fe_inst_load(
            ig->f, 
            expr->entity->fe_ty,
            expr->entity->extra,
            FE_MEMOP_ALIGN_DEFAULT,
            0
        ));
        return load;
    } break;
    default:
        UNREACHABLE;
    }
}

static FeInst* irgen_expr_integer_binop(IRGen* ig, Expr* expr) {
    FeInst* lhs = irgen_expr(ig, expr->binary.lhs);
    FeInst* rhs = irgen_expr(ig, expr->binary.rhs);

    FeInstKindGeneric kind;
    switch (expr->kind) {
    case EXPR_ADD:
        kind = FE_IADD;
        break;
    case EXPR_SUB:
        kind = FE_ISUB;
        break;
    default:
        UNREACHABLE;
    }

    FeInst* binop = fe_append_end(ig->b, fe_inst_binop(ig->f, lhs->ty, kind, lhs, rhs));
    return binop;
}

static FeInst* irgen_expr(IRGen* ig, Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
        return irgen_expr_literal(ig, expr);
    case EXPR_ENTITY:
        return irgen_expr_entity(ig, expr);
    case EXPR_ADD:
    case EXPR_SUB:
        return irgen_expr_integer_binop(ig, expr);
    default:
        UNREACHABLE;
    }

    UNREACHABLE;
}

static void irgen_stmt_return(IRGen* ig, Stmt* stmt) {
    assert(ig->f->sig->return_len == 1);

    FeInst* ret_expr = irgen_expr(ig, stmt->expr);
    FeInst* ret = fe_append_end(ig->b, fe_inst_return(ig->f));
    fe_return_set_arg(ig->f, ret, 0, ret_expr);
}

static void irgen_stmt(IRGen* ig, Stmt* stmt) {
    switch (stmt->kind) {
    case STMT_RETURN:
        irgen_stmt_return(ig, stmt);
        break;
    default:
        UNREACHABLE;
    }
}

static void irgen_function(IRGen* ig) {
    Stmt* function_decl = ig->e->decl;

    // generate stack slots for parameters
    for_n(i, 0, function_decl->fn_decl.parameters.len) {
        Entity* entity = function_decl->fn_decl.parameters.at[i];
        
        // TODO dont recalculate this, get it from earlier somewhere
        TySelectResult iron_ty = select_iron_type(entity->ty);        
        FeStackItem* stack_slot = fe_stack_append_top(ig->f, fe_stack_item_new(iron_ty.ty, iron_ty.cty));
        // entity->extra = stack_slot;

        FeInst* stackaddr = fe_append_end(ig->b, fe_inst_stack_addr(ig->f, stack_slot));
        entity->extra = stackaddr;

        // store into the stack slot
        FeInst* store = fe_append_end(ig->b, fe_inst_store(ig->f, 
            stackaddr, 
            fe_func_param(ig->f, i), 
            FE_MEMOP_ALIGN_DEFAULT, 0
        ));
    }

    // generate each statement in turn
    for_n(i, 0, function_decl->fn_decl.body.len) {
        Stmt* stmt = function_decl->fn_decl.body.stmts[i];
        irgen_stmt(ig, stmt);
    }

    fe_solve_mem_pessimistic(ig->f);
    
}

static TySelectResult select_iron_type(TyIndex ty_index) {
    FeTy ty = TY_VOID;
    FeComplexTy* cty = nullptr;
    switch (TY_KIND(ty_index)) {
    case TY_BYTE: 
    case TY_UBYTE:
        ty = FE_TY_I8;
        break;
    case TY_INT: 
    case TY_UINT:
        ty = FE_TY_I16;
        break;
    case TY_LONG: 
    case TY_ULONG:
        ty = FE_TY_I32;
        break;
    case TY_QUAD:
    case TY_UQUAD:
        ty = FE_TY_I64;
        break;
    default:
        UNREACHABLE;
    }

    return (TySelectResult){ty, nullptr};
}

static FeFuncSig* generate_signature(CompilationUnit* cu, FeModule* m, Entity* e) {
    u16 param_len = 0;
    u16 return_len = 0;

    TyFn* fn_ty = TY(e->ty, TyFn);

    for_n(i, 0, fn_ty->len) {
        Ty_FnParam* parameter = &fn_ty->params[i];
        if (parameter->out) {
            return_len += 1;
        } else {
            param_len += 1;
        }
    }
    if (fn_ty->ret_ty != TY_VOID) {
        return_len += 1;
    }
    if (fn_ty->variadic) {
        // param_len += 2;
        UNREACHABLE;
    }

    FeFuncSig* sig = fe_funcsig_new(FE_CCONV_JACKAL, param_len, return_len);

    u16 current_param = 0;
    u16 current_return = 0;

    if (fn_ty->ret_ty != TY_VOID) {
        
        FeFuncParam* fe_return_parameter = fe_funcsig_return(sig, current_return);        
        TySelectResult iron_ty = select_iron_type(fn_ty->ret_ty);
        fe_return_parameter->ty = iron_ty.ty;
        fe_return_parameter->cty = iron_ty.cty;

        current_return += 1;
    }

    for_n(i, 0, fn_ty->len) {
        Ty_FnParam* parameter = &fn_ty->params[i];
    
        FeFuncParam* fe_parameter = !parameter->out 
            ? fe_funcsig_param(sig, current_param)
            : fe_funcsig_return(sig, current_return)
        ;
        
        // decide parameter's type
        TySelectResult iron_ty = select_iron_type(parameter->ty);
        fe_parameter->ty = iron_ty.ty;
        fe_parameter->cty = iron_ty.cty;

        if (!parameter->out) {
            current_param += 1;
        } else {
            current_return += 1;
        }
    }

    return sig;
}
