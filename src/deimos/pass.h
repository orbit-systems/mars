#pragma once
#define DEIMOS_PASS_H

#include "../phobos/ast.h"
#include "passes/passes.h"
#include "deimos_ir.h"
#include "deimos_ssa.h"

void run_passes(AST base_node);

void init_passes();

IR pass_legalise(AST base_node);

typedef enum {
	PASS_IR_2_IR,
	PASS_SSA_2_SSA,
	//PASS_DAG_2_SSA,
	//PASS_SSA_2_SSA,
	PASS_ID_COUNT,
} pass_id;

typedef struct {
	union {
		void   (*callback)();
		da(IR) (*IR_2_IR)(da(IR));
		SSA    (*SSA_2_SSA)(SSA);
		//FIXME: include SSA stuff
	};
	string pass_name;
	pass_id id;
} pass;



da_typedef(pass);