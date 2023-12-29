#include "orbit.h"
#include "tac.h"


TAC_element* createTACelement(TAC_type type, int argcount, TAC_arg* arguments) {
	if (argcount < 0 || argcount > 3) return NULL;
	TAC_element int_elem = (TAC_element){.type=type, .arg_len=argcount};
	for (int i = 0; i < argcount; i++) int_elem.arg[i] = arguments[i];
	

}

void testTAC() {
	createTACelement(label_function, 1, &(TAC_arg){label, string_make("main", strlen("main"))});
	//createTACelement()
	//createTACelement()
	//createTACelement()
	//createTACelement()
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