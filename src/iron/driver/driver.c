#define ORBIT_IMPLEMENTATION

#include "iron/iron.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

int main() {

    FeModule* m = fe_new_module(str("cfg"));

    FeSymbol* fn_sym = fe_new_symbol(m, str("foo"), FE_BIND_EXPORT);
    FeFunction* fn = fe_new_function(m, fn_sym);

    fe_init_func_params(fn, 2);
    fe_add_func_param(fn, fe_type(m, FE_TYPE_I64));
    fe_add_func_param(fn, fe_type(m, FE_TYPE_I64));

    fe_init_func_returns(fn, 1);
    fe_add_func_return(fn, fe_type(m, FE_TYPE_I64));

    /*
    fn foo(x: int) -> int {
        mut y: int;
        if x == 1 {
            y = 3;
        }
        return y;
    }
    */

    FeStackObject* var_x = fe_new_stackobject(fn, fe_type(m, FE_TYPE_I64));
    FeStackObject* var_y = fe_new_stackobject(fn, fe_type(m, FE_TYPE_I64));
    
    FeBasicBlock* entry_bb = fe_new_basic_block(fn, str("entry"));
    FeBasicBlock* if_bb = fe_new_basic_block(fn, str("if_true"));
    FeBasicBlock* return_bb = fe_new_basic_block(fn, str("return"));

    // entry bb
    {
        FeInstParamVal* x_paramval = (FeInstParamVal*) fe_append(entry_bb, fe_inst_paramval(fn, 0));
        FeInstStackStore* x_init_store = (FeInstStackStore*) fe_append(entry_bb, fe_inst_stack_store(fn, var_x, (FeInst*)x_paramval));

        FeInstConst* zero = (FeInstConst*) fe_append(entry_bb, fe_inst_const(fn, fe_type(m, FE_TYPE_I64)));
        zero->i64 = 0ul;

        FeInstStackStore* y_init_store = (FeInstStackStore*) fe_append(entry_bb, fe_inst_stack_store(fn, var_y, (FeInst*)zero));

        // load and compare
        FeInst* x_load = fe_append(entry_bb, fe_inst_stack_load(fn, var_x));

        FeInstConst* one = (FeInstConst*) fe_append(entry_bb, fe_inst_const(fn, fe_type(m, FE_TYPE_I64)));
        one->i64 = 1ul;

        FeInst* compare_eq = fe_append(entry_bb, fe_inst_binop(fn, FE_INST_EQ, x_load, (FeInst*) one));
        FeInst* branch = fe_append(entry_bb, fe_inst_branch(fn, compare_eq, if_bb, return_bb));
    }

    // if bb
    {
        FeInstConst* three = (FeInstConst*) fe_append(if_bb, fe_inst_const(fn, fe_type(m, FE_TYPE_I64)));
        three->i64 = 3ul;

        FeInstStackStore* y_store = (FeInstStackStore*) fe_append(if_bb, fe_inst_stack_store(fn, var_y, (FeInst*)three));
        FeInst* jump = fe_append(if_bb, fe_inst_jump(fn, return_bb));

    }

    // return bb
    {
        FeInst* y_load = fe_append(return_bb, fe_inst_stack_load(fn, var_y));

        FeInst* returnval = fe_append(return_bb, fe_inst_returnval(fn, 0, y_load));
        FeInst* ret = fe_append(return_bb, fe_inst_return(fn));
    }

    fe_sched_pass(m, &fe_pass_stackprom);

    fe_run_all_passes(m, true);

    printstr(fe_emit_ir(m));
}