#include "common/portability.h"
#include "common/strmap.h"
#include "common/util.h"

#include "irgen.h"
#include "iron/iron.h"
#include "lex.h"
#include "parse.h"
#include <assert.h>
#include <string.h>

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
        FeSymbolKind kind = FE_SYMKIND_NONE;
        switch (e->kind) {
        case ENTKIND_FN:
            kind = FE_SYMKIND_FUNC;
            break;
        case ENTKIND_VAR:
            kind = FE_SYMKIND_STATIC;
            break;
        default:
            UNREACHABLE;
        }

        // FeSection* section = e->kind == ENTKIND_FN ? default_text : default_data;
        FeSection* section = ig->default_text;
        FeSymbol* sym = fe_symbol_new(ig->m, from_compact(e->name).raw, from_compact(e->name).len, section, bind);
        sym->kind = kind;
    }
    
    // generate static data
    for_n(i, 0, cu->top_scope->map.cap) {
        Entity* e = cu->top_scope->map.vals[i];
        if (e == nullptr) {
            continue;
        }

        if (e->kind != ENTKIND_VAR) {
            continue;
        }
        
        FeSymbol* sym = fe_symtab_get(&ig->m->symtab, from_compact(e->name).raw, from_compact(e->name).len);
        assert(sym != nullptr);

        TySelectResult fe_ty = select_iron_type(ig, e->ty);

        FeStatic* s = fe_static_new(ig->m, sym, fe_ty.ty, fe_ty.cty);

        // TODO("generate static variable '"str_fmt"'", str_arg(from_compact(e->name)));
    }
    
    // generate functions
    for_n(i, 0, cu->top_scope->map.cap) {
        Entity* e = cu->top_scope->map.vals[i];
        if (e == nullptr) {
            continue;
        }

        if (e->kind != ENTKIND_FN) {
            continue;
        }

        FeSymbol* sym = fe_symtab_get(&ig->m->symtab, from_compact(e->name).raw, from_compact(e->name).len);
        assert(sym != nullptr);

        // ig->e = e;

        FeFuncSig* signature = generate_signature(ig, e->ty);

        if (e->storage == STORAGE_EXTERN) {
            continue;
        }

        FeFunc* func = fe_func_new(ig->m, sym, signature, ipool, vregs);

        ig->f = func;
        ig->b = func->entry_block;

        irgen_function(ig, e->decl);
    }

    return ig->m;
}

typedef struct AddrInfo {
    FeInst* val;
    FeAliasCategory cat;
} AddrInfo;

static AddrInfo irgen_addr(IRGen* ig, Expr* expr);

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
    usize from_size = fe_ty_get_size(ig->m->target, expr->ty, nullptr);
    usize to_size = fe_ty_get_size(ig->m->target, to, nullptr);
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
        fe_amap_add(ig->amap, load->id, expr->entity->alias_cat);

        return load;
    }
    case STORAGE_PUBLIC:
    case STORAGE_PRIVATE:
    case STORAGE_EXTERN: {
        string name = from_compact(expr->entity->name);
        FeSymbol* sym = fe_symtab_get(&ig->m->symtab, name.raw, name.len);
        assert(sym != nullptr);

        FeInst* addr = fe_append_end(ig->b, fe_inst_sym_addr(ig->f, sym));
        if (sym->kind == FE_SYMKIND_FUNC) {
            // function, get the address of the function itself
            return addr;
        }
        // static data
        assert(sym->stat->cty == nullptr);
        FeInst* load = fe_append_end(ig->b, fe_inst_load(
            ig->f, 
            sym->stat->ty, 
            addr,
            FE_MEMOP_ALIGN_DEFAULT,
            0
        ));
        // fe_inst_load(f)
        fe_amap_add(ig->amap, load->id, ig->alias.memory);
        return load;
    }
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
    case EXPR_MEMBER: {
        AddrInfo member_addr = irgen_addr(ig, expr);
        TySelectResult member_ty = select_iron_type(ig, expr->ty);
        assert(member_ty.cty == nullptr);
        FeInst* load = fe_append_end(ig->b, fe_inst_load(
            ig->f,
            member_ty.ty,
            member_addr.val,
            FE_MEMOP_ALIGN_DEFAULT,
            0
        ));
        fe_amap_add(ig->amap, load->id, member_addr.cat);
        return load;
    }
    case EXPR_DEREF_MEMBER: {
        FeInst* member_addr = irgen_value(ig, expr);
        TySelectResult member_ty = select_iron_type(ig, expr->ty);
        assert(member_ty.cty == nullptr);
        FeInst* load = fe_append_end(ig->b, fe_inst_load(
            ig->f,
            member_ty.ty,
            member_addr,
            FE_MEMOP_ALIGN_DEFAULT,
            0
        ));
        fe_amap_add(ig->amap, load->id, ig->alias.memory);
        return load;
    }
    case EXPR_CALL: {
        FeInst* function_addr = irgen_value(ig, expr->call.callee);

        FeInst* arg_values[expr->call.args_len];
        for_n(i, 0, expr->call.args_len) {
            arg_values[i] = irgen_value(ig, expr->call.args[i]);
        }

        TyIndex fn_ty = expr->call.callee->ty;
        if (TY_KIND(fn_ty) == TY_PTR) {
            fn_ty = TY(fn_ty, TyPtr)->to;
        }
        assert(TY_KIND(fn_ty) == TY_FN);

        FeFuncSig* sig = generate_signature(ig, fn_ty);

        FeInst* call = fe_append_end(ig->b, 
            fe_inst_call(ig->f, function_addr, sig)
        );
        fe_amap_add(ig->amap, call->id, ig->alias.memory);
        for_n(i, 0, expr->call.args_len) {
            fe_call_set_arg(ig->f, call, i, arg_values[i]);
        }

        FeInst* returnval = nullptr;

        // manage return values
        if (sig->return_len > 0) {
            returnval = fe_append_end(ig->b,
                fe_inst_proj(ig->f, call, 0)
            );
        }
        for_n(i, 1, sig->return_len) {
            TODO("assign OUT parameters");
        }
        
        return returnval;

    }
    default:
        CRASH("failed to generate value expression %d", expr->kind);
        // UNREACHABLE;
    }
}

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
        return (AddrInfo){expr->entity->extra, expr->entity->alias_cat};
    } break;
    default:
        TODO("storage kind %u", expr->entity->storage);
    }
}

static AddrInfo irgen_addr(IRGen* ig, Expr* expr) {
    switch (expr->kind) {
    case EXPR_ENTITY:
        return irgen_addr_entity(ig, expr);
    case EXPR_DEREF:
        return (AddrInfo){irgen_value(ig, expr->unary), ig->alias.memory};
    case EXPR_MEMBER: {
        TySelectResult aggregate_ty = select_iron_type(ig, 
            expr->member_access.aggregate->ty
        );
        usize offset = aggregate_ty.cty->record.fields[expr->member_access.member_index].offset;

        AddrInfo aggregate_addr = irgen_addr(ig, expr->member_access.aggregate);
        
        FeInst* member_addr = fe_append_end(ig->b, fe_inst_ptr_offset(
            ig->f, 
            aggregate_addr.val, 
            offset
        ));
        return (AddrInfo){member_addr, aggregate_addr.cat};
    }
    case EXPR_DEREF_MEMBER: {
        TyIndex aggregate_ptr_ty = expr->member_access.aggregate->ty;

        TySelectResult aggregate_ty = select_iron_type(ig, 
            TY(aggregate_ptr_ty, TyPtr)->to
        );
        usize offset = aggregate_ty.cty->record.fields[expr->member_access.member_index].offset;

        FeInst* aggregate_addr = irgen_value(ig, expr->member_access.aggregate);
        
        FeInst* member_addr = fe_append_end(ig->b, fe_inst_ptr_offset(
            ig->f, 
            aggregate_addr, 
            offset
        ));
        return (AddrInfo){member_addr, ig->alias.memory};
    }
    default:
        TODO("expr kind %u", expr->kind);
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
    fe_amap_add(ig->amap, ret->id, ig->alias.memory);
    fe_return_set_arg(ig->f, ret, 0, ret_expr);

    return STMTINFO(true);
}

static StmtInfo irgen_stmt_vardecl(IRGen* ig, Stmt* stmt) {
    Entity* var = stmt->var_decl.var;
    var->alias_cat = fe_amap_new_category(ig->amap);

    // create stack slot
    TySelectResult iron_ty = select_iron_type(ig, var->ty);
    var->fe_ty = iron_ty.ty;
    var->fe_cty = iron_ty.cty;
    // assert(var->fe_cty == nullptr);
    usize ty_size = fe_ty_get_size(ig->m->target, iron_ty.ty, iron_ty.cty);
    usize ty_align = fe_ty_get_align(ig->m->target, iron_ty.ty, iron_ty.cty);

    FeStackItem* stack_slot = fe_stack_append_top(ig->f, fe_stack_item_new(ty_size, ty_align));

    FeInst* stackaddr = fe_append_end(ig->b, fe_inst_stack_addr(ig->f, stack_slot));
    var->extra = stackaddr;

    bool var_is_complex_ty = var->fe_cty != nullptr;

    if (stmt->var_decl.expr != nullptr) {
        if (!var_is_complex_ty) {
            FeInst* value = irgen_value(ig, stmt->var_decl.expr);
            value = irgen_integer_cast(ig, value, iron_ty.ty);

            FeInst* store = fe_append_end(ig->b, fe_inst_store(ig->f,
                stackaddr,
                value,
                FE_MEMOP_ALIGN_DEFAULT,
                0
            ));
            fe_amap_add(ig->amap, store->id, var->alias_cat);
        } else {
            TODO("compound initializer");
        }
    }


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

static void irgen_function(IRGen* ig, Stmt* function_decl) {

    FeAliasMap amap;
    fe_amap_init(&amap);
    ig->amap = &amap;
    ig->alias.memory = fe_amap_new_category(ig->amap);

    // generate stack slots for parameters
    for_n(i, 0, vec_len(function_decl->fn_decl.parameters)) {
        Entity* entity = function_decl->fn_decl.parameters[i];
        if (entity->alias_cat._ == 0) {
            entity->alias_cat = fe_amap_new_category(ig->amap);
        }

        // TODO dont recalculate this, get it from earlier somewhere
        TySelectResult iron_ty = select_iron_type(ig, entity->ty);
        usize ty_size = fe_ty_get_size(ig->m->target, iron_ty.ty, iron_ty.cty);
        usize ty_align = fe_ty_get_align(ig->m->target, iron_ty.ty, iron_ty.cty);

        FeStackItem* stack_slot = fe_stack_append_top(ig->f, fe_stack_item_new(ty_size, ty_align));

        FeInst* stackaddr = fe_append_end(ig->b, fe_inst_stack_addr(ig->f, stack_slot));
        entity->extra = stackaddr;

        // store into the stack slot
        FeInst* store = fe_append_end(ig->b, fe_inst_store(ig->f,
            stackaddr,
            fe_func_param(ig->f, i),
            FE_MEMOP_ALIGN_DEFAULT, 0
        ));
        fe_amap_add(ig->amap, store->id, entity->alias_cat);
    }

    // generate each statement in turn
    StmtInfo info;
    for_n(i, 0, function_decl->fn_decl.body.len) {
        Stmt* stmt = function_decl->fn_decl.body.stmts[i];
        info = irgen_stmt(ig, stmt);
        if (info.diverges) {
            break;
        }
    }
    if (!info.diverges) {
        // create a return
        FeInst* ret = fe_append_end(ig->b, fe_inst_return(ig->f));
        fe_amap_add(ig->amap, ret->id, ig->alias.memory);
    }

    // fe_opt_reorder_blocks_rpo(ig->f);
    fe_construct_domtree(ig->f);

    fe_solve_mem(ig->f, &amap);

    fe_amap_destroy(&amap);
}

static TySelectResult create_iron_type(IRGen* ig, TyIndex ty_index) {

    switch (TY_KIND(ty_index)) {
    case TY_PTR:
        return (TySelectResult){FE_TY_PTR, nullptr};
    case TY_ALIAS:
        return select_iron_type(ig, TY(ty_index, TyAlias)->aliasing);
    case TY_ENUM:
        return select_iron_type(ig, TY(ty_index, TyEnum)->backing_ty);
    case TY_STRUCT: {
        TyRecord* rec = TY(ty_index, TyRecord);
        FeComplexTy* cty = fe_ty_record_new(rec->len);
        for_n(i, 0, rec->len) {
            cty->record.fields[i].offset = rec->members[i].offset;
            TySelectResult member_ty = select_iron_type(ig, rec->members[i].type);
            cty->record.fields[i].ty = member_ty.ty;
            cty->record.fields[i].complex_ty = member_ty.cty;
        }
        return (TySelectResult){FE_TY_RECORD, cty};
    }
    case TY_UNION: {
        TyRecord* rec = TY(ty_index, TyRecord);
        FeComplexTy* cty = fe_ty_record_new(rec->len);
        for_n(i, 0, rec->len) {
            cty->record.fields[i].offset = 0;
            TySelectResult member_ty = select_iron_type(ig, rec->members[i].type);
            cty->record.fields[i].ty = member_ty.ty;
            cty->record.fields[i].complex_ty = member_ty.cty;
        }
        return (TySelectResult){FE_TY_RECORD, cty};
    }
    default:
        TODO("cannot translate frontend type %u with kind %u to iron type", ty_index, TY_KIND(ty_index));
    }

}

static TySelectResult select_iron_type(IRGen* ig, TyIndex ty_index) {

    static TySelectResult* select_cache = nullptr;

    assert(ty_index != TY_VOID);

    // initialize the first time
    if_unlikely (select_cache == nullptr) {
        select_cache = malloc(sizeof(select_cache[0]) * tybuf.len);
        memset(select_cache, 0, sizeof(select_cache[0]) * tybuf.len);

        select_cache[TY_VOID]  = (TySelectResult){FE_TY_VOID, nullptr};
        select_cache[TY_BYTE]  = (TySelectResult){FE_TY_I8, nullptr};
        select_cache[TY_UBYTE] = (TySelectResult){FE_TY_I8, nullptr};
        select_cache[TY_INT]   = (TySelectResult){FE_TY_I16, nullptr};
        select_cache[TY_UINT]  = (TySelectResult){FE_TY_I16, nullptr};
        select_cache[TY_LONG]  = (TySelectResult){FE_TY_I32, nullptr};
        select_cache[TY_ULONG] = (TySelectResult){FE_TY_I32, nullptr};
        select_cache[TY_QUAD]  = (TySelectResult){FE_TY_I64, nullptr};
        select_cache[TY_UQUAD] = (TySelectResult){FE_TY_I64, nullptr};
    }

    TySelectResult select = select_cache[ty_index];
    if (select.ty == FE_TY_VOID) {
        select = create_iron_type(ig, ty_index);
        select_cache[ty_index] = select;
    }

    return select;
}

static FeFuncSig* generate_signature(IRGen* ig, TyIndex ty) {
    u16 param_len = 0;
    u16 return_len = 0;
    
    // printf("tyname %s\n", ty_name(ty));
    assert(TY_KIND(ty) == TY_FN);
    TyFn* fn_ty = TY(ty, TyFn);

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
