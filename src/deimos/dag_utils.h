#pragma once

#include "deimos_dag.h"

typedef struct {
	DAG* node;
	DAG* parent;
} extracted_dag_node;

da_typedef(extracted_dag_node);

da(extracted_dag_node) extract_dag_nodes(DAG base_node, dag_type type);