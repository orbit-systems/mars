#pragma once
#define DEIMOS_PASS_H

typedef enum {
	PASS_IR_TO_IR,
	PASS_MI_TO_MI,
} pass_type;

typedef struct {
	char* name;
	union {
		void (*callback)();
		IR_Module* (*ir_callback)(IR_Module*);
		//MInst->MInst
	};
	pass_type type;
} Pass;

da_typedef(Pass);

da(Pass) deimos_passes;