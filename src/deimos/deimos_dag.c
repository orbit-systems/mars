#include "deimos_dag.h"

size_t dag_type_size[] = {
    0,
#define DAG_TYPE(ident, identstr, structdef) sizeof(dag_##ident),
    DAG_NODES
#undef DAG_TYPE
    0
};

char* dag_type_str[] = {
    "invalid",
#define DAG_TYPE(ident, identstr, structdef) identstr,
    DAG_NODES
#undef DAG_TYPE
    "COUNT",
};

// allocate and zero a new DAG node with an arena
DAG new_dag_node(arena* restrict alloca, dag_type type, int depth) {
    DAG node;
    void* node_ptr = arena_alloc(alloca, dag_type_size[type], 8);
    if (node_ptr == NULL) {
        general_error("internal: new_dag_node() could not allocate DAG node of type '%s' with size %d", dag_type_str[type], dag_type_size[type]);
    }
    memset(node_ptr, 0, dag_type_size[type]);
    node.rawptr = node_ptr;
    node.type = type;
    node.depth = depth;
    return node;
}