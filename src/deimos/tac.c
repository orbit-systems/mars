#include "orbit.h"
#include "tac.h"

dynarr_lib(TAC_element);

dynarr(TAC_element) tac_elems;


TAC_element* createTACelement(TAC_type type, int argcount, TAC_arg* arguments) {
	if (argcount < 0 || argcount > 3) return NULL;

	TAC_element int_elem = (TAC_element){.type=type, .arg_len=argcount};
	for (int i = 0; i < argcount; i++) int_elem.arg[i] = arguments[i];
	
	return NULL;

	if (tac_elems.base == NULL) dynarr_init(TAC_element, &tac_elems, 1);
	dynarr_append(TAC_element, &tac_elems, int_elem);
}

void testTAC() {
	createTACelement(label_function, 1, &(TAC_arg){label,       to_string("main")});

	createTACelement(load_data,      2,  (TAC_arg[]){
									 	 (TAC_arg){tac_var,    to_string("t0")},
										 (TAC_arg){literal_int, to_string("1")}
										 			});

	createTACelement(load_data,      2,  (TAC_arg[]){
										 (TAC_arg){tac_var,    to_string("t1")},
										 (TAC_arg){literal_int, to_string("1")}
													});

	createTACelement(operator_add,   3,  (TAC_arg[]){
										 (TAC_arg){tac_var,    to_string("t2")},
										 (TAC_arg){tac_var,    to_string("t1")},
										 (TAC_arg){tac_var,    to_string("t0")}
													});

	createTACelement(function_return, 0, NULL);
}

char* asmText = NULL;

void appendTextToAsm(char* string) {
	asmText = realloc(asmText, sizeof(*asmText) * (strlen(asmText) + strlen(string)));
	asmText = strcat(asmText, string); //leaky
}

void processTAC() {
	//we now emit valid assembly
	string currentLabel = NULL_STR;
	for (int i = 0; i < tac_elems.len; i++) {
		TAC_element int_elem = tac_elems.base[i];
		switch(int_elem.type) {
			case label_function:
				printf("FUNCTION %s -> %s:\n", int_elem.arg[0].value.raw, int_elem.arg[0].value.raw);
				char buffer[1027];
				if (strlen(int_elem.arg[0].value.raw) > (1026 - strlen(":\n"))) CRASH("label was longer than 1024 characters.");
				sprintf(buffer, "%s:\n", int_elem.arg[0].value.raw);
				appendTextToAsm(buffer);
				break;
			default:
				printf("tac type %d not impl'd.\n", int_elem.type);
				TODO("tac type not impl'd!");
		}

		printf("current asm:\n%s\n\n", asmText);
	}
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