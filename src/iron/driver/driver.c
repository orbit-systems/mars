#define ORBIT_IMPLEMENTATION

#include "iron/iron.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

int main() {

    FeModule* m = fe_new_module(str("cfg"));

    {
        FeSymbol* fn_sym = fe_new_symbol(m, str("foo"), FE_BIND_EXPORT);
        FeFunction* fn = fe_new_function(m, fn_sym);

        fe_init_func_params(fn, 1);
        fe_add_func_param(fn, FE_TYPE_BOOL);

        fe_init_func_returns(fn, 1);
        fe_add_func_return(fn, FE_TYPE_I64);

        /*
        fn foo(x: bool) -> int {
            mut y: int = 0;
            if x {
                y = 3;
            }
            return y;
        }
        */

        FeStackObject* var_y = fe_new_stackobject(fn, FE_TYPE_I64);
        
        FeBasicBlock* entry_bb = fe_new_basic_block(fn, str("entry"));
        FeBasicBlock* if_bb = fe_new_basic_block(fn, str("if_true"));
        FeBasicBlock* return_bb = fe_new_basic_block(fn, str("return"));

        FeInst* x_paramval =  fe_append(entry_bb, fe_inst_paramval(fn, 0));

        // entry bb
        {
            FeInstConst* zero = (FeInstConst*) fe_append(entry_bb, fe_inst_const(fn, FE_TYPE_I64));
            zero->i64 = 0ul;
            FeInstStackStore* y_init_store = (FeInstStackStore*) fe_append(entry_bb, fe_inst_stack_store(fn, var_y, (FeInst*)zero));
            FeInst* branch = fe_append(entry_bb, fe_inst_branch(fn, x_paramval, if_bb, return_bb));
        }

        // if bb
        {
            FeInstConst* three = (FeInstConst*) fe_append(if_bb, fe_inst_const(fn, FE_TYPE_I64));
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
    }

    {
        /*
                    D
                   / \
            A-->--B   E-->--F
             \     \ /     /
              \     C     /
               ----->-----
        */

        FeSymbol* fn_sym = fe_new_symbol(m, str("bar"), FE_BIND_EXPORT);
        FeFunction* fn = fe_new_function(m, fn_sym);

        FeBasicBlock* A = fe_new_basic_block(fn, str("A"));
        FeBasicBlock* B = fe_new_basic_block(fn, str("B"));
        FeBasicBlock* C = fe_new_basic_block(fn, str("C"));
        FeBasicBlock* D = fe_new_basic_block(fn, str("D"));
        FeBasicBlock* E = fe_new_basic_block(fn, str("E"));
        FeBasicBlock* F = fe_new_basic_block(fn, str("F"));

        fe_init_func_params(fn, 1);
        fe_add_func_param(fn, FE_TYPE_BOOL);

        FeInst* cond = fe_append(A, fe_inst_paramval(fn, 0));
        fe_append(A, fe_inst_branch(fn, cond, B, F));
        fe_append(B, fe_inst_branch(fn, cond, C, D));
        fe_append(C, fe_inst_jump(fn, E));
        fe_append(D, fe_inst_jump(fn, E));
        fe_append(E, fe_inst_jump(fn, F));
        fe_append(F, fe_inst_return(fn));
    }











    fe_sched_pass(m, &fe_pass_stackprom);

    fe_run_all_passes(m, true);

    // printstr(fe_emit_ir(m));
}