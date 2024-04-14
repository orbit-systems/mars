#include "../deimos.h"

DAG pass_dag2tdag(DAG base_node) {
	//this pass takes all decl_stmt nodes and transforms them into assign statements.
	//these are either depth 0 nodes, or a child of a block_stmt.
	da(extracted_dag_node) extracted_nodes = extract_dag_nodes(base_node, dagtype_decl_stmt);
	general_warning("FIXME: god is cruel");
}