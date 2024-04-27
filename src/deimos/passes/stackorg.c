#include "deimos.h"
//this pass moves all stack allocations to the TOP of the IR, so that targets can extract stack frame info

//this pass uses qsort to sort each IR_Function and each IR_BasicBlock

int sort_stackalloc(const void* a, const void* b) {
	IR* ir_a = *(IR**)a;
	IR* ir_b = *(IR**)b;
	if (ir_a->tag == IR_STACKALLOC && ir_b->tag != IR_STACKALLOC) return -1;
	if (ir_a->tag == IR_STACKALLOC && ir_b->tag == IR_STACKALLOC) {
		if (type_real_size_of(ir_a->T) > type_real_size_of(ir_b->T)) return -1;
		else if (type_real_size_of(ir_a->T) < type_real_size_of(ir_b->T)) return 1;
		else return 0;
	}
	if (ir_a->tag != IR_STACKALLOC && ir_b->tag != IR_STACKALLOC) return 0;
}

IR_Module* ir_pass_stackorg(IR_Module* mod) {
	FOR_URANGE(i, 0, mod->functions_len) {
		FOR_URANGE(j, 0, mod->functions[i]->blocks.len) {
			printf("Sorting BB of length %d\n", mod->functions[i]->blocks.at[j]->len);
			qsort(mod->functions[i]->blocks.at[j]->at, mod->functions[i]->blocks.at[j]->len, sizeof(IR**), sort_stackalloc);
		}
	}
	return mod;
}