#include "orbit.h"
#include "pass.h"

//implements the pass system for processing the aphelion AST and transforming it into a DAG

//da(pass) passes;

void add_pass() {

}

void init_passes() {

}

void run_passes(AST base_node) {
	//execute passes in order of init
	//the legalising pass is special, and doesnt actually take in a DAG, it takes in the AST.
	
	DAG legalised_dag = pass_legalise(base_node);
}