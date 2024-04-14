#pragma once
#define DEIMOS_PASS_H

#include "../phobos/ast.h"
#include "deimos_dag.h"
#include "passes/passes.h"

void run_passes(AST base_node);

void init_passes();

DAG pass_legalise(AST base_node);

typedef enum {
	PASS_DAG_2_DAG,
	//PASS_DAG_2_SSA,
	//PASS_SSA_2_SSA,
	PASS_ID_COUNT,
} pass_id;

typedef struct {
	union {
		void (*callback)();
		DAG  (*DAG_2_DAG)(DAG);
		//FIXME: include SSA stuff
	};
	string pass_name;
	pass_id id;
} pass;



da_typedef(pass);