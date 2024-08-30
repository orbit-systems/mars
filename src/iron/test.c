#include "iron/iron.h"
#include "passes/passes.h"

void test_algsimp_reassoc() {
    printf("\n");

    FeModule* m = fe_new_module(str("test"));

    FeSymbol* sym = fe_new_symbol(m, str("algsimp_test"), FE_BIND_LOCAL);
    FeFunction* f = fe_new_function(m, sym);
    fe_set_func_params(f, 1, fe_type(m, FE_TYPE_I64));
    fe_set_func_returns(f, 1, fe_type(m, FE_TYPE_I64));

    FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));

    FeInstParamVal* p = (FeInstParamVal*) fe_append(bb, fe_inst_paramval(f, 0));
    
    FeInstConst* c1 = (FeInstConst*) fe_append(bb,
        fe_inst_const(f)
    ); c1->base.type = fe_type(m, FE_TYPE_I64); c1->i64 = 5;

    FeInstConst* c2 = (FeInstConst*) fe_append(bb,
        fe_inst_const(f)
    ); c2->base.type = fe_type(m, FE_TYPE_I64); c2->i64 = 9;

    FeInst* add = fe_append(bb, 
        fe_inst_binop(f, FE_INST_ADD, (FeInst*) p, (FeInst*) c1)
    ); add->type = fe_type(m, FE_TYPE_I64);

    FeInst* add2 = fe_append(bb, 
        fe_inst_binop(f, FE_INST_ADD, (FeInst*) add, (FeInst*) c2)
    ); add2->type = fe_type(m, FE_TYPE_I64);

    fe_append(bb, fe_inst_returnval(f, 0, (FeInst*) add2));
    fe_append(bb, fe_inst_return(f));

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
    FeFunction* f = fe_new_function(m, sym);
    fe_set_func_params(f, 1, fe_type(m, FE_TYPE_I64));
    fe_set_func_returns(f, 1, fe_type(m, FE_TYPE_I64));

    FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));

    FeInstParamVal* p = (FeInstParamVal*) fe_append(bb, fe_inst_paramval(f, 0));
    
    FeInstConst* c1 = (FeInstConst*) fe_append(bb,
        fe_inst_const(f)
    ); c1->base.type = fe_type(m, FE_TYPE_I64); c1->i64 = 16;

    FeInst* mul = fe_append(bb, 
        fe_inst_binop(f, FE_INST_UMUL, (FeInst*) p, (FeInst*) c1)
    ); mul->type = fe_type(m, FE_TYPE_I64);

    fe_append(bb, fe_inst_returnval(f, 0, (FeInst*) mul));
    fe_append(bb, fe_inst_return(f));

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
    FeFunction* f = fe_new_function(m, sym);
    fe_set_func_params(f, 2, fe_type(m, FE_TYPE_I64), fe_type(m, FE_TYPE_I64));
    fe_set_func_returns(f, 2, fe_type(m, FE_TYPE_I64), fe_type(m, FE_TYPE_I64));

    FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));
    FeInstParamVal* p0 = (FeInstParamVal*) fe_append(bb, fe_inst_paramval(f, 0));
    FeInstParamVal* p1 = (FeInstParamVal*) fe_append(bb, fe_inst_paramval(f, 1));

    FeInst* add = fe_append(bb, fe_inst_binop(f, FE_INST_ADD, (FeInst*) p0, (FeInst*) p1)); 
    add->type = fe_type(m, FE_TYPE_I64);

    FeInst* mul = fe_append(bb, fe_inst_binop(f, FE_INST_UMUL, (FeInst*) p0, (FeInst*) p1)); 
    mul->type = fe_type(m, FE_TYPE_I64);

    FeInst* r0 = fe_append(bb, fe_inst_returnval(f, 0, (FeInst*) add));
    FeInst* r1 = fe_append(bb, fe_inst_returnval(f, 1, (FeInst*) mul));

    fe_append(bb, fe_inst_return(f));

    fe_run_all_passes(m, true);

    fe_emit_c(m);

    fe_destroy_module(m);
}

// conduct a full self-test. bugs that impede functionality should be caught here. 
void fe_selftest() {
    test_c_gen();
    
}