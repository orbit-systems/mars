#include "iron/iron.h"
#include <stdio.h>

static void quick_print(FeFunc* f) {
    FeDataBuffer db;
    fe_db_init(&db, 512);
    fe_emit_ir_func(&db, f, true);
    printf("%.*s", (int)db.len, db.at);
}

FeFunc* make_store_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeSection* text = fe_section_new(mod, "text", 0, FE_SECTION_EXECUTABLE);

    FeTy ptr_ty = mod->target->ptr_ty;

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_ANY, 2, 1);
    fe_funcsig_param(f_sig, 0)->ty = ptr_ty;
    fe_funcsig_param(f_sig, 1)->ty = ptr_ty;
    fe_funcsig_return(f_sig, 0)->ty = ptr_ty;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "foo", 0, text, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;

    FeInst* store = fe_inst_store(
        f, 
        fe_func_param(f, 0), 
        fe_func_param(f, 1), 
        FE_MEMOP_ALIGN_DEFAULT, 0
    );
    fe_append_end(entry, store);

    FeInst* load = fe_inst_load(
        f, 
        ptr_ty, 
        fe_func_param(f, 0), 
        FE_MEMOP_ALIGN_DEFAULT, 0
    );
    fe_append_end(entry, load);

    FeInst* zero = fe_inst_const(f, ptr_ty, 0);
    fe_append_end(entry, zero);

    FeInst* zero_store = fe_inst_store(
        f, 
        fe_func_param(f, 0), 
        zero, 
        FE_MEMOP_ALIGN_DEFAULT, 0
    );
    fe_append_end(entry, zero_store);

    FeInst* ret = fe_inst_return(f);
    fe_append_end(entry, ret);
    fe_return_set_arg(f, ret, 0, load);
    
    fe_solve_mem_pessimistic(f);

    return f;
}

FeFunc* make_alg_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeSection* text = fe_section_new(mod, "text", 0, FE_SECTION_EXECUTABLE);

    FeTy ptr_ty = mod->target->ptr_ty;

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_ANY, 1, 1);
    fe_funcsig_param(f_sig, 0)->ty = ptr_ty;
    fe_funcsig_return(f_sig, 0)->ty = ptr_ty;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "foo", 0, text, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;

    FeInst* x = fe_inst_const(f, ptr_ty, 10);
    fe_append_end(entry, x);

    FeInst* shift_x = fe_inst_binop(f,
        ptr_ty, FE_IADD, 
        fe_func_param(f, 0),
        x
    );
    fe_append_end(entry, shift_x);

    FeInst* ret = fe_inst_return(f);
    fe_append_end(entry, ret);
    fe_return_set_arg(f, ret, 0, shift_x);
    
    fe_solve_mem_pessimistic(f);

    return f;
}

FeFunc* make_struct_test(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {
    FeSection* text = fe_section_new(mod, "text", 0, FE_SECTION_EXECUTABLE);

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_JACKAL, 1, 1);
    fe_funcsig_param(f_sig, 0)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 0)->ty = FE_TY_I32;

    FeSymbol* f_sym = fe_symbol_new(mod, "foo", 0, text, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;
    FeTy ptr_ty = mod->target->ptr_ty;

    /*
        struct Foo {
            i8 x;
            i32 y;
        };

        u32 foo(u32 x) {
            Foo f = {x, x};
            return f.x;
        }
    */

    FeComplexTy* struct_ty = fe_ty_record_new(2);
    struct_ty->record.fields[0].ty = FE_TY_I8;
    struct_ty->record.fields[0].complex_ty = nullptr;
    struct_ty->record.fields[0].offset = 0;
    struct_ty->record.fields[1].ty = FE_TY_I32;
    struct_ty->record.fields[1].complex_ty = nullptr;
    struct_ty->record.fields[1].offset = 4;

    FeStackItem* item = fe_stack_append_top(f, fe_stack_item_new(8, 4));

    FeInst* param0 = fe_func_param(f, 0);
    FeInst* stackaddr = fe_append_end(entry, fe_inst_stack_addr(f, item));
    FeInst* trunc = fe_append_end(entry, fe_inst_unop(f, FE_TY_I8, FE_TRUNC, param0));
    FeInst* store_i8 = fe_append_end(entry, fe_inst_store(f, stackaddr, trunc, FE_MEMOP_ALIGN_DEFAULT, 0));
    FeInst* i32_offset = fe_append_end(entry, fe_inst_const(f, ptr_ty, 4));
    FeInst* i32_addr = fe_append_end(entry, fe_inst_binop(f, ptr_ty, FE_IADD, stackaddr, i32_offset));
    // FeInst* store_i32 = fe_append_end(entry, `fe_inst_store(f, i32_addr, param0, FE_MEMOP_ALIGN_DEFAULT, 0));
    FeInst* load_i32 = fe_append_end(entry, fe_inst_load(f, FE_TY_I32, i32_addr, FE_MEMOP_ALIGN_DEFAULT, 0));
    
    FeInst* ret = fe_append_end(entry, fe_inst_return(f));
    fe_return_set_arg(f, ret, 0, load_i32);

    fe_solve_mem_pessimistic(f);

    return f;
}


FeFunc* make_store_test2(FeModule* mod, FeInstPool* ipool, FeVRegBuffer* vregs) {

    /*
        fun foo(ptr, val: i32): i32 {
            ptr^ = val;
            (ptr + 8)^ = val;
            return ptr^;
        }
    */

    FeSection* text = fe_section_new(mod, "text", 0, FE_SECTION_EXECUTABLE);

    FeTy ptr_ty = mod->target->ptr_ty;

    FeFuncSig* f_sig = fe_funcsig_new(FE_CCONV_ANY, 2, 1);
    fe_funcsig_param(f_sig, 0)->ty = ptr_ty;
    fe_funcsig_param(f_sig, 1)->ty = FE_TY_I32;
    fe_funcsig_return(f_sig, 0)->ty = ptr_ty;

    // make the function and its symbol
    FeSymbol* f_sym = fe_symbol_new(mod, "foo", 0, text, FE_BIND_GLOBAL);
    FeFunc* f = fe_func_new(mod, f_sym, f_sig, ipool, vregs);

    FeBlock* entry = f->entry_block;

    auto ptr = fe_func_param(f, 0);
    auto val = fe_func_param(f, 1);

    auto store1 = fe_append_end(entry, fe_inst_store(f, ptr, val, FE_MEMOP_ALIGN_DEFAULT, 0));
    auto const8 = fe_append_end(entry, fe_inst_const(f, ptr->ty, 8));
    auto ptr_offset = fe_append_end(entry, fe_inst_binop(f, ptr->ty, FE_IADD, ptr, const8));
    auto store2 = fe_append_end(entry, fe_inst_store(f, ptr_offset, val, FE_MEMOP_ALIGN_DEFAULT, 0));

    auto load1 = fe_append_end(entry, fe_inst_load(f, ptr_ty, ptr, FE_MEMOP_ALIGN_DEFAULT, 0));

    FeInst* ret = fe_inst_return(f);
    fe_append_end(entry, ret);
    fe_return_set_arg(f, ret, 0, load1);
    
    fe_solve_mem_pessimistic(f);

    return f;
}

int main() {
    fe_init_signal_handler();
    FeInstPool ipool;
    fe_ipool_init(&ipool);
    FeVRegBuffer vregs;
    fe_vrbuf_init(&vregs, 2048);

    FeModule* mod = fe_module_new(FE_ARCH_XR17032, FE_SYSTEM_FREESTANDING);

    FeFunc* func = make_store_test2(mod, &ipool, &vregs);

    quick_print(func);
    fe_opt_local(func);
    quick_print(func);

    fe_codegen(func);
    quick_print(func);

    // printf("------ final assembly ------\n");

    // FeDataBuffer db; 
    // fe_db_init(&db, 2048);
    // fe_emit_asm(&db, mod);
    // printf("%.*s", (int)db.len, db.at);

    // fe_module_destroy(mod);
}
