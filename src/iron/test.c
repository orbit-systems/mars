#include "iron/iron.h"
#include "passes/passes.h"

void test_algsimp_reassoc() {
    printf("\n");

    FeModule* m = fe_new_module(str("test"));

    FeSymbol* sym = fe_new_symbol(m, str("algsimp_test"), FE_BIND_LOCAL);
    FeFunction* f = fe_new_function(m, sym, FE_CCONV_MARS);
    fe_init_func_params(f, 1);
    fe_add_func_param(f, FE_TYPE_I64);
    fe_init_func_returns(f, 1);
    fe_add_func_return(f, FE_TYPE_I64);

    FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));

    FeIrParamVal* p = (FeIrParamVal*) fe_append_ir(bb, fe_ir_paramval(f, 0));
    
    FeIrConst* c1 = (FeIrConst*) fe_append_ir(bb,
        fe_ir_const(f, FE_TYPE_I64)
    ); c1->i64 = 5;

    FeIrConst* c2 = (FeIrConst*) fe_append_ir(bb,
        fe_ir_const(f, FE_TYPE_I64)
    ); c2->i64 = 9;

    FeIr* add = fe_append_ir(bb, 
        fe_ir_binop(f, FE_IR_ADD, (FeIr*) p, (FeIr*) c1)
    ); add->type = FE_TYPE_I64;

    FeIr* add2 = fe_append_ir(bb, 
        fe_ir_binop(f, FE_IR_ADD, (FeIr*) add, (FeIr*) c2)
    ); add2->type = FE_TYPE_I64;

    fe_append_ir(bb, fe_ir_returnval(f, 0, (FeIr*) add2));
    fe_append_ir(bb, fe_ir_return(f));

    fe_sched_pass(m, &fe_pass_algsimp);
    fe_sched_pass(m, &fe_pass_tdce);
    fe_run_all_passes(m, true);

    fe_emit_c(m);
    // string s = fe_emit_ir(m);
    // printf(str_fmt, str_arg(s));
    fe_destroy_module(m);
}

void test_algsimp_sr() {
    printf("\n");

    FeModule* m = fe_new_module(str("test"));

    FeSymbol* sym = fe_new_symbol(m, str("algsimp_test"), FE_BIND_LOCAL);
    FeFunction* f = fe_new_function(m, sym, FE_CCONV_MARS);
    fe_init_func_params(f, 1);
    fe_add_func_param(f, FE_TYPE_I64);
    fe_init_func_returns(f, 1);
    fe_add_func_return(f, FE_TYPE_I64);

    FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));

    FeIrParamVal* p = (FeIrParamVal*) fe_append_ir(bb, fe_ir_paramval(f, 0));
    
    FeIrConst* c1 = (FeIrConst*) fe_append_ir(bb,
        fe_ir_const(f, FE_TYPE_I64)
    ); c1->i64 = 16;

    FeIr* mul = fe_append_ir(bb, 
        fe_ir_binop(f, FE_IR_UMUL, (FeIr*) p, (FeIr*) c1)
    ); mul->type = FE_TYPE_I64;

    fe_append_ir(bb, fe_ir_returnval(f, 0, (FeIr*) mul));
    fe_append_ir(bb, fe_ir_return(f));

    fe_sched_pass(m, &fe_pass_algsimp);
    fe_sched_pass(m, &fe_pass_tdce);
    fe_run_all_passes(m, true);

    // string s = fe_emit_ir(m);
    // printf(str_fmt, str_arg(s));

    fe_emit_c(m);

    fe_destroy_module(m);
}

void test_c_gen() {
    printf("\n");

    FeModule* m = fe_new_module(str("test"));

    FeSymbol* sym = fe_new_symbol(m, str("add_mul"), FE_BIND_LOCAL);
    FeFunction* f = fe_new_function(m, sym, FE_CCONV_MARS);
    fe_init_func_params(f, 2);
    fe_add_func_param(f, FE_TYPE_I64);
    fe_add_func_param(f, FE_TYPE_I64);
    fe_init_func_returns(f, 2);
    fe_add_func_return(f, FE_TYPE_I64);
    fe_add_func_return(f, FE_TYPE_I64);

    FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));
    FeIrParamVal* p0 = (FeIrParamVal*) fe_append_ir(bb, fe_ir_paramval(f, 0));
    FeIrParamVal* p1 = (FeIrParamVal*) fe_append_ir(bb, fe_ir_paramval(f, 1));

    FeIr* add = fe_append_ir(bb, fe_ir_binop(f, FE_IR_ADD, (FeIr*) p0, (FeIr*) p1)); 
    add->type = FE_TYPE_I64;

    FeIr* mul = fe_append_ir(bb, fe_ir_binop(f, FE_IR_UMUL, (FeIr*) p0, (FeIr*) p1)); 
    mul->type = FE_TYPE_I64;

    FeIr* r0 = fe_append_ir(bb, fe_ir_returnval(f, 0, (FeIr*) add));
    FeIr* r1 = fe_append_ir(bb, fe_ir_returnval(f, 1, (FeIr*) mul));

    fe_append_ir(bb, fe_ir_return(f));

    fe_run_all_passes(m, true);

    fe_emit_c(m);

    fe_destroy_module(m);
}

// conduct a full self-test. bugs that impede functionality should be caught here. 
void fe_selftest() {
    test_c_gen();
    
}

//TODO: change .reg_count to be calculated by log_2((.defs | .uses) + 1) - 1 (assumption is .defs | .uses is po2 - 1)