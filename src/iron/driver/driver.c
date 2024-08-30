#define ORBIT_IMPLEMENTATION

#include "iron/iron.h"

// application frontend for iron. this is not included when
// building iron as a library or as part of mars.

int main() {

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

}