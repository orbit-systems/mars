#include "dag_utils.h"

void edn_walk_dag(DAG node, DAG parent, DAG grandparent, 
				  da(extracted_dag_node) extracted_nodes, dag_type extracted_type, int depth);

da(extracted_dag_node) extract_dag_nodes(DAG base_node, dag_type type) {
	//this extracts all dag nodes of dag_type type, and places them in a dynamic array pointing to that node.
	da(extracted_dag_node) extracted_nodes;
	da_init(&extracted_nodes, 1);
	edn_walk_dag(base_node, NULL_DAG, NULL_DAG, extracted_nodes, type, 0);
	return extracted_nodes;
}

void edn_walk_dag(DAG node, DAG parent, DAG grandparent, 
				  da(extracted_dag_node) extracted_nodes, dag_type extracted_type, int depth) {
	if (node.type == extracted_type) {
		//da_append(&extracted_nodes, (extracted_dag_node){})
		return;
	}
	switch (node.type) {
		case dagtype_entry: {
			printf("we out here meowing\n");
			edn_walk_dag(node.as_entry->node, node, parent, extracted_nodes, extracted_type, depth + 1);
			break;
		}
		default:
			if (node.type > dagtype_COUNT) {
				general_error("edn_walk_dag | Depth: %d | Encountered OOB DAG node: %d", depth, node.type);
				break;
			}
			general_error("edn_walk_dag | Depth: %d | Encountered unhandled DAG node: %s", depth, dag_type_str[node.type]);
			break;
	}
}