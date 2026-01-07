#include "irgen.h"
#include "iron/iron.h"
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
    ig->default_rodata = fe_section_new(ig->m, "bss", 6, FE_SECTION_BLANK);

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

        FeSymbolBinding bind = FE_BIND_GLOBAL;
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
        FeSymbol* sym = fe_symbol_new(ig->m, from_compact(e->name).raw, from_compact(e->name).len, section, bind);
    
        ig->e = e;

        FeFuncSig* signature = generate_signature(ig);

        FeFunc* func = fe_func_new(ig->m, sym, signature, ipool, vregs);

        ig->f = func;
        ig->b = func->entry_block;

        irgen_function(ig);
    }

    return ig->m;
}

#define TY(index, T) ((T*)&tybuf.at[index])
#define TY_KIND(index) ((TyBase*)&tybuf.at[index])->kind

static FeInst* irgen_value_literal(IRGen* ig, Expr* expr) {
    assert(expr->ty <= TY_UQUAD);

    TySelectResult iron_ty = select_iron_type(ig, expr->ty);
    FeInst* lit = fe_append_end(ig->b, fe_inst_const(ig->f, iron_ty.ty, expr->literal));
    return lit;
}

static FeInst* irgen_integer_cast(IRGen* ig, FeInst* expr, FeTy to) {
    if (expr->ty == to) {
        return expr;
    }
    usize from_size = fe_ty_get_size(expr->ty, nullptr);
    usize to_size = fe_ty_get_size(to, nullptr);
    bool signext = ty_is_signed(to);

    
    if (from_size > to_size) {
        // truncation
        return fe_append_end(ig->b, fe_inst_unop(ig->f, 
            to, 
            FE_TRUNC, 
            expr
        ));
    } else {
        if (signext) {
            return fe_append_end(ig->b, fe_inst_unop(ig->f, 
                to, 
                FE_SIGN_EXT, 
                expr
            ));
        } else {
            return fe_append_end(ig->b, fe_inst_unop(ig->f, 
                to, 
                FE_ZERO_EXT, 
                expr
            ));
        }
    }
}

static FeInst* irgen_value_entity(IRGen* ig, Expr* expr) {
    switch (expr->entity->storage) {
    case STORAGE_LOCAL: {
        expr->entity->fe_ty = select_iron_type(ig, expr->ty).ty;

        FeInst* load = fe_append_end(ig->b, fe_inst_load(
            ig->f, 
            expr->entity->fe_ty,
            expr->entity->extra,
            FE_MEMOP_ALIGN_DEFAULT,
            0
        ));
        fe_amap_add(ig->amap, load->id, ig->alias.stack);

        return load;
    } break;
    default:
        UNREACHABLE;
    }
}

static FeInst* irgen_value_integer_binop(IRGen* ig, Expr* expr) {
    FeInst* lhs = irgen_value(ig, expr->binary.lhs);
    FeInst* rhs = irgen_value(ig, expr->binary.rhs);

    rhs = irgen_integer_cast(ig, rhs, lhs->ty);

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
static FeInst* irgen_value_load(IRGen* ig, Expr* expr) {
    FeInst* ptr = irgen_value(ig, expr->unary);

    TySelectResult fe_ty = select_iron_type(ig, expr->ty);
    assert(fe_ty.cty == nullptr);

    FeInst* load = fe_append_end(ig->b, fe_inst_load(ig->f, 
        fe_ty.ty, 
        ptr, 
        FE_MEMOP_ALIGN_DEFAULT,
        0
    ));
    fe_amap_add(ig->amap, load->id, ig->alias.memory);
    return load;
}

static FeInst* irgen_value(IRGen* ig, Expr* expr) {
    switch (expr->kind) {
    case EXPR_LITERAL:
        return irgen_value_literal(ig, expr);
    case EXPR_ENTITY:
        return irgen_value_entity(ig, expr);
    case EXPR_ADD:
    case EXPR_SUB:
        return irgen_value_integer_binop(ig, expr);
    case EXPR_DEREF:
        return irgen_value_load(ig, expr);
    default:
        UNREACHABLE;
    }
}

typedef struct AddrInfo {
    FeInst* val;
    FeAliasCategory cat;
} AddrInfo;

static AddrInfo irgen_addr_entity(IRGen* ig, Expr* expr) {
    switch (expr->entity->storage) {
    case STORAGE_LOCAL: {
        expr->entity->fe_ty = select_iron_type(ig, expr->ty).ty;

        // FeInst* load = fe_append_end(ig->b, fe_inst_load(
        //     ig->f, 
        //     expr->entity->fe_ty,
        //     expr->entity->extra,
        //     FE_MEMOP_ALIGN_DEFAULT,
        //     0
        // ));
        return (AddrInfo){expr->entity->extra, ig->alias.stack};
    } break;
    default:
        UNREACHABLE;
    }
}

static AddrInfo irgen_addr(IRGen* ig, Expr* expr) {
    switch (expr->kind) {
    case EXPR_ENTITY:
        return irgen_addr_entity(ig, expr);
    case EXPR_DEREF:
        return (AddrInfo){irgen_value(ig, expr->unary), ig->alias.memory};
    default:
        UNREACHABLE;
    }
}

static StmtInfo irgen_stmt_return(IRGen* ig, Stmt* stmt) {
    assert(ig->f->sig->return_len == 1);

    FeInst* ret_expr = irgen_value(ig, stmt->expr);
    ret_expr = irgen_integer_cast(ig, 
        ret_expr,
        fe_funcsig_return(ig->f->sig, 0)->ty
    );
    FeInst* ret = fe_append_end(ig->b, fe_inst_return(ig->f));
    fe_return_set_arg(ig->f, ret, 0, ret_expr);

    return STMTINFO(true);
}

static StmtInfo irgen_stmt_vardecl(IRGen* ig, Stmt* stmt) {
    Entity* var = stmt->var_decl.var;
    
    // create stack slot
    TySelectResult iron_ty = select_iron_type(ig, var->ty);        
    var->fe_ty = iron_ty.ty;
    var->fe_cty = iron_ty.cty;
    assert(var->fe_cty == nullptr);
    
    FeStackItem* stack_slot = fe_stack_append_top(ig->f, fe_stack_item_new(iron_ty.ty, iron_ty.cty));

    FeInst* stackaddr = fe_append_end(ig->b, fe_inst_stack_addr(ig->f, stack_slot));
    var->extra = stackaddr;

    FeInst* value = irgen_value(ig, stmt->var_decl.expr);
    value = irgen_integer_cast(ig, value, iron_ty.ty);

    FeInst* store = fe_append_end(ig->b, fe_inst_store(ig->f, 
        stackaddr, 
        value,
        FE_MEMOP_ALIGN_DEFAULT,
        0
    ));
    fe_amap_add(ig->amap, store->id, ig->alias.stack);

    return STMTINFO(false);
}

static StmtInfo irgen_stmt_assign(IRGen* ig, Stmt* stmt) {
    Expr* lhs = stmt->assign.lhs;
    Expr* rhs = stmt->assign.rhs;
    
    TySelectResult iron_ty = select_iron_type(ig, lhs->ty);        
    assert(iron_ty.cty == nullptr);

    AddrInfo addr = irgen_addr(ig, lhs);
    FeInst* value = irgen_value(ig, rhs);

    FeInst* store = fe_append_end(ig->b, fe_inst_store(ig->f, addr.val, value, FE_MEMOP_ALIGN_DEFAULT, 0));
    fe_amap_add(ig->amap, store->id, addr.cat);

    return STMTINFO(false);
}

static StmtInfo irgen_stmt_if(IRGen* ig, Stmt* stmt) {
    assert(stmt->if_.else_ == nullptr);

    // Expr* cond = stmt->if_.cond;
    FeInst* cond = irgen_value(ig, stmt->if_.cond);
    
    if (cond->ty != FE_TY_BOOL) {
        FeInst* zero = fe_append_end(ig->b, fe_inst_const(ig->f, cond->ty, 0));
        FeInst* eq = fe_append_end(ig->b, fe_inst_binop(ig->f, 
            FE_TY_BOOL, 
            FE_IEQ, 
            cond, 
            zero
        ));
        cond = eq;
    }

    FeBlock* branch_block = ig->b;

    FeBlock* true_block = fe_block_new(ig->f);

    bool true_diverges = false;
    // codegen on true block
    ig->b = true_block;
    for_n(i, 0, stmt->if_.block.len) {
        Stmt* substmt = stmt->if_.block.stmts[i];
        true_diverges = irgen_stmt(ig, substmt).diverges;
        if (true_diverges) {
            break;
        }
    }
    FeBlock* true_end = ig->b;

    FeBlock* false_block = fe_block_new(ig->f);

    FeInst* branch = fe_append_end(branch_block, fe_inst_branch(ig->f, cond));

    fe_branch_set_false(ig->f, branch, true_block);
    fe_branch_set_true(ig->f, branch, false_block);

    if (!true_diverges) {
        // jump from true block to false block
        FeInst* jump_back = fe_append_end(true_end, fe_inst_jump(ig->f));
        fe_jump_set_target(ig->f, jump_back, false_block);
    }
    
    // continue to codegen on false block
    ig->b = false_block;

    return STMTINFO(false);
}

static StmtInfo irgen_stmt_while(IRGen* ig, Stmt* stmt) {

    FeBlock* condition_block = fe_block_new(ig->f);

    FeInst* into_condition = fe_append_end(ig->b, fe_inst_jump(ig->f));
    fe_jump_set_target(ig->f, into_condition, condition_block);

    ig->b = condition_block;

    // generate condition
    FeInst* cond = irgen_value(ig, stmt->while_.cond);
    
    if (cond->ty != FE_TY_BOOL) {
        FeInst* zero = fe_append_end(ig->b, fe_inst_const(ig->f, cond->ty, 0));
        FeInst* eq = fe_append_end(ig->b, fe_inst_binop(ig->f, 
            FE_TY_BOOL, 
            FE_IEQ, 
            cond, 
            zero
        ));
        cond = eq;
    }

    FeBlock* loop_block = fe_block_new(ig->f);
    ig->b = loop_block;

    bool loop_diverges = false;
    for_n(i, 0, stmt->while_.block.len) {
        Stmt* substmt = stmt->while_.block.stmts[i];
        loop_diverges = irgen_stmt(ig, substmt).diverges;
        if (loop_diverges) {
            break;
        }
    }
    FeBlock* loop_end = ig->b;

    if (!loop_diverges) {
        // jump from loop end to condition block
        FeInst* jump_back = fe_append_end(loop_end, fe_inst_jump(ig->f));
        fe_jump_set_target(ig->f, jump_back, condition_block);
    }

    FeBlock* break_block = fe_block_new(ig->f);

    FeInst* branch = fe_append_end(condition_block, fe_inst_branch(ig->f, cond));

    fe_branch_set_false(ig->f, branch, loop_block);
    fe_branch_set_true(ig->f, branch, break_block);

    ig->b = break_block;

    return STMTINFO(false);
}

static StmtInfo irgen_stmt_barrier(IRGen* ig, Stmt* stmt) {
    FeInst* vol_effect = fe_append_end(ig->b, fe_inst_mem_barrier(ig->f));
    fe_amap_add(ig->amap, vol_effect->id, ig->alias.memory);

    return STMTINFO(false);
}

static StmtInfo irgen_stmt(IRGen* ig, Stmt* stmt) {
    switch (stmt->kind) {
    case STMT_BARRIER:
        return irgen_stmt_barrier(ig, stmt);
    case STMT_RETURN:
        return irgen_stmt_return(ig, stmt);
    case STMT_VAR_DECL:
        return irgen_stmt_vardecl(ig, stmt);
    case STMT_ASSIGN:
        return irgen_stmt_assign(ig, stmt);
    case STMT_IF:
        return irgen_stmt_if(ig, stmt);
    case STMT_WHILE:
        return irgen_stmt_while(ig, stmt);
    default:
        UNREACHABLE;
    }
}

static void irgen_function(IRGen* ig) {
    Stmt* function_decl = ig->e->decl;

    FeAliasMap amap;
    fe_amap_init(&amap);
    ig->alias.stack = fe_amap_new_category(&amap);
    ig->alias.memory = fe_amap_new_category(&amap);
    ig->amap = &amap;

    // generate stack slots for parameters
    for_n(i, 0, vec_len(function_decl->fn_decl.parameters)) {
        Entity* entity = function_decl->fn_decl.parameters[i];
        
        // TODO dont recalculate this, get it from earlier somewhere
        TySelectResult iron_ty = select_iron_type(ig, entity->ty);        
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
        fe_amap_add(ig->amap, store->id, ig->alias.stack);
    }

    // generate each statement in turn
    for_n(i, 0, function_decl->fn_decl.body.len) {
        Stmt* stmt = function_decl->fn_decl.body.stmts[i];
        irgen_stmt(ig, stmt);
    }

    // fe_opt_reorder_blocks_rpo(ig->f);
    fe_construct_domtree(ig->f);

    fe_solve_mem(ig->f, &amap);

    fe_amap_destroy(&amap);
}

static TySelectResult select_iron_type(IRGen* ig, TyIndex ty_index) {
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
    case TY_PTR:
        ty = ig->m->target->ptr_ty;
        break;
    default:
        UNREACHABLE;
    }

    return (TySelectResult){ty, nullptr};
}

static FeFuncSig* generate_signature(IRGen* ig) {
    u16 param_len = 0;
    u16 return_len = 0;

    TyFn* fn_ty = TY(ig->e->ty, TyFn);

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
        TySelectResult iron_ty = select_iron_type(ig, fn_ty->ret_ty);
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
        TySelectResult iron_ty = select_iron_type(ig, parameter->ty);
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
