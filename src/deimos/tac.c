#include "orbit.h"
#include "tac.h"


TAC_element* createTACelement(TAC_type type, int argcount, TAC_arg* arguments) {
	if (argcount < 0 || argcount > 3) return NULL;
	TAC_element int_elem = (TAC_element){.type=type, .arg_len=argcount};
	for (int i = 0; i < argcount; i++) int_elem.arg[i] = arguments[i];
	
	return NULL;
}

void testTAC() {
	createTACelement(label_function, 1, &(TAC_arg){label,       to_string("main")});

	createTACelement(load_data,      2,  (TAC_arg[]){
									 	 (TAC_arg){tac_data,    to_string("t0")},
										 (TAC_arg){literal_int, to_string("1")}
										 			});

	createTACelement(load_data,      2,  (TAC_arg[]){
										 (TAC_arg){tac_data,    to_string("t1")},
										 (TAC_arg){literal_int, to_string("1")}
													});

	createTACelement(operator_add,   3,  (TAC_arg[]){
										 (TAC_arg){tac_data,    to_string("t2")},
										 (TAC_arg){tac_data,    to_string("t1")},
										 (TAC_arg){tac_data,    to_string("t0")}
													});

	createTACelement(function_return, 0, NULL);
}
/*
let main = fn() {
	let a = 1 + 2;
	return;
};

FUNCTION main
LOAD_LITERAL t0, 1
LOAD_LITERAL t1, 2
ADD t2, t1, t0
RETURN

main:
	li ra, 1
	li rb, 2
	add rc, rb, ra
	ret
*/