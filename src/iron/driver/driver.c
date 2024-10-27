#define ORBIT_IMPLEMENTATION

#include "iron/iron.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

int main() {

    FeModule* m = fe_new_module(str("cfg"));

    {
         FeSymbol* sym = fe_new_symbol(m, str("algsimp_test"), FE_BIND_LOCAL);
        FeFunction* f = fe_new_function(m, sym, FE_CCONV_MARS);
        fe_init_func_params(f, 1);
        fe_add_func_param(f, FE_TYPE_I64);
        fe_init_func_returns(f, 1);
        fe_add_func_return(f, FE_TYPE_I64);

        FeBasicBlock* bb = fe_new_basic_block(f, str("block1"));

        FeIrParam* p = (FeIrParam*)fe_append_ir(bb, fe_ir_param(f, 0));

        FeIrConst* c1 = (FeIrConst*)fe_append_ir(bb, fe_ir_const(f, FE_TYPE_I64));
        c1->i64 = 1;

        FeIrConst* c2 = (FeIrConst*)fe_append_ir(bb, fe_ir_const(f, FE_TYPE_I64));
        c2->i64 = 2;

        FeIr* add = fe_append_ir(bb, fe_ir_binop(f, FE_IR_ADD, (FeIr*)c1, (FeIr*)c2));
        add->type = FE_TYPE_I64;

        FeIr* add2 = fe_append_ir(bb, fe_ir_binop(f, FE_IR_ADD, (FeIr*)add, (FeIr*)c2));
        add2->type = FE_TYPE_I64;

        FeIrReturn* ret = (FeIrReturn*)fe_append_ir(bb, fe_ir_return(f));
        ret->sources[0] = (FeIr*)add2;
    }

    if (0) {
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

        FeIr* x_param =  fe_append_ir(entry_bb, fe_ir_param(fn, 0));

        // entry bb
        {
            FeIrConst* zero = (FeIrConst*) fe_append_ir(entry_bb, fe_ir_const(fn, FE_TYPE_I64));
            zero->i64 = 0ul;
            FeIrStackStore* y_init_store = (FeIrStackStore*) fe_append_ir(entry_bb, fe_ir_stack_store(fn, var_y, (FeIr*)zero));
            FeIr* branch = fe_append_ir(entry_bb, fe_ir_branch(fn, x_param, if_bb, return_bb));
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


    fe_sched_module_pass(m, &fe_pass_algsimp);

    fe_run_all_passes(m, true);

    // printstr(fe_emit_ir(m));
}