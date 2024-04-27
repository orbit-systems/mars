#include "deimos.h"
//this pass moves all stack allocations to the TOP of the IR, so that targets can extract stack frame info

//this pass uses qsort to sort each IR_Function and each IR_BasicBlock

int sort_stackalloc(const void* a, const void* b) {

}

IR_Module* ir_pass_stackorg(IR_Module* mod) {
	FOR_URANGE(i, 0, mod.)
}