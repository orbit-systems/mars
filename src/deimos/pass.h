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

void run_passes(IR_Module* current_program);
void register_passes();

da_typedef(Pass);

extern da(Pass) deimos_passes;