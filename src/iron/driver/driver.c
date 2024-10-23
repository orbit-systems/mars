#define ORBIT_IMPLEMENTATION

#include "iron/iron.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

int main() {

    FeModule* m = fe_new_module(str("cfg"));

    {
        FeSymbol* fn_sym = fe_new_symbol(m, str("foo"), FE_BIND_EXPORT);
        FeFunction* fn = fe_new_function(m, fn_sym, FE_CCONV_MARS);

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

        FeIr* x_paramval =  fe_append_ir(entry_bb, fe_ir_paramval(fn, 0));

        // entry bb
        {
            FeIrConst* zero = (FeIrConst*) fe_append_ir(entry_bb, fe_ir_const(fn, FE_TYPE_I64));
            zero->i64 = 0ul;
            FeIrStackStore* y_init_store = (FeIrStackStore*) fe_append_ir(entry_bb, fe_ir_stack_store(fn, var_y, (FeIr*)zero));
            FeIr* branch = fe_append_ir(entry_bb, fe_ir_branch(fn, x_paramval, if_bb, return_bb));
        }

        // if bb
        {
            FeIrConst* three = (FeIrConst*) fe_append_ir(if_bb, fe_ir_const(fn, FE_TYPE_I64));
            three->i64 = 3ul;

            FeIrStackStore* y_store = (FeIrStackStore*) fe_append_ir(if_bb, fe_ir_stack_store(fn, var_y, (FeIr*)three));
            FeIr* jump = fe_append_ir(if_bb, fe_ir_jump(fn, return_bb));

        }

        // return bb
        {
            FeIr* y_load = fe_append_ir(return_bb, fe_ir_stack_load(fn, var_y));

            FeIrReturn* ret = (FeIrReturn*) fe_append_ir(return_bb, fe_ir_return(fn));
            ret->sources[0] = y_load;
        }
    }

    if (0) {
        /*
                    D
                   / \
            A-->--B   E-->--F
             \     \ /     /
              \     C     /
               ----->-----
        */

        FeSymbol* fn_sym = fe_new_symbol(m, str("bar"), FE_BIND_EXPORT);
        FeFunction* fn = fe_new_function(m, fn_sym, FE_CCONV_CDECL);

        FeBasicBlock* A = fe_new_basic_block(fn, str("A"));
        FeBasicBlock* B = fe_new_basic_block(fn, str("B"));
        FeBasicBlock* C = fe_new_basic_block(fn, str("C"));
        FeBasicBlock* D = fe_new_basic_block(fn, str("D"));
        FeBasicBlock* E = fe_new_basic_block(fn, str("E"));
        FeBasicBlock* F = fe_new_basic_block(fn, str("F"));

        fe_init_func_params(fn, 1);
        fe_add_func_param(fn, FE_TYPE_BOOL);

        FeIr* cond = fe_append_ir(A, fe_ir_paramval(fn, 0));
        fe_append_ir(A, fe_ir_branch(fn, cond, B, F));
        fe_append_ir(B, fe_ir_branch(fn, cond, C, D));
        fe_append_ir(C, fe_ir_jump(fn, E));
        fe_append_ir(D, fe_ir_jump(fn, E));
        fe_append_ir(E, fe_ir_jump(fn, F));
        fe_append_ir(F, fe_ir_return(fn));
    }




    fe_sched_module_pass(m, &fe_pass_stackprom);

    fe_run_all_passes(m, true);

    // printstr(fe_emit_ir(m));
}