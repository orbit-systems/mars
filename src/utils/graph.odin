package mars_utils

import "core:fmt"

edge_type :: enum {
	DIRECTED,
	NON_DIRECTED
}

edge :: struct {
	vertices: [2]int,
	type:	  edge_type,
	weight:   f32,
}

vertex :: struct {
	edges: [dynamic]int,
	type:  int, //node type
}

graph :: struct {
	edges : [dynamic]edge,
	vertices : [dynamic]^vertex,
}

createGraph :: proc() -> graph {
	intGraph: graph
	return intGraph
}

santizeGraph :: proc(intGraph: ^graph) {
	//edge santitisation
	for edge, i in intGraph.edges {
		if (edge.vertices[0] > (len(intGraph.vertices) - 1)) || edge.vertices[0] < 0 {
			fmt.printf("Edge {} on connection 0 is invalid, pointing to {}. defaulting to vertex 0!", i, edge.vertices[0])
			intGraph.edges[i].vertices[0] = 0
		}

		if (edge.vertices[1] > (len(intGraph.vertices) - 1)) || edge.vertices[1] < 0 {
			fmt.printf("Edge {} on connection 1 is invalid, pointing to {}. defaulting to vertex 0!", i, edge.vertices[1])
			intGraph.edges[i].vertices[1] = 0
		}
	}

	for vertex, i in intGraph.vertices {
		clear(&intGraph.vertices[i].edges)
	}

	//fix vertices
	for edge, i in intGraph.edges {
		append(&intGraph.vertices[edge.vertices[0]].edges, i)
		append(&intGraph.vertices[edge.vertices[1]].edges, i)
	}
}

addEdge :: proc(intEdge: edge, intGraph: ^graph) {
	intEdge := intEdge

	if (intEdge.vertices[0] > (len(intGraph.vertices) - 1)) || intEdge.vertices[0] < 0 {
		intEdge.vertices[0] = 0
	}

	if (intEdge.vertices[1] > (len(intGraph.vertices) - 1)) || intEdge.vertices[1] < 0 {
		intEdge.vertices[1] = 0
	}

	append(&intGraph.edges, intEdge)
	append(&intGraph.vertices[intEdge.vertices[0]].edges, len(intGraph.edges) - 1)
	append(&intGraph.vertices[intEdge.vertices[1]].edges, len(intGraph.edges) - 1)
}

addVertex :: proc(intGraph: ^graph) {
	intVertex := new(vertex)
	append(&intGraph.vertices, intVertex)
}

MST :: proc(intGraph: graph) -> graph {
	return intGraph
}

hasCycle :: proc(intGraph: graph) {
	finished, visited: [dynamic]^vertex
	defer delete(finished)
	defer delete(visited)

	for vertex in intGraph.vertices {
		DFS(vertex, intGraph, finished, visited)
	}
}

DFS :: proc(intVertex: ^vertex, intGraph: graph, finished, visited: [dynamic]^vertex) {
	finished := finished
	visited  := visited

	//get vertex id
	id := ---
	for vertexIter, i in intGraph.vertices {
		if vertexIter == intVertex {
			id = i
		}
	}

	for vertexIter in finished { //if finished(v)
		if vertexIter == intVertex {
			return
		}
	}

	for vertexIter in visited { //if visited(v)
		if vertexIter == intVertex {
			fmt.printf("Found cycle on id: {}!\n", id)
			return
		}
	}

	append(&visited, intVertex)
	neighbour_list: [dynamic]^vertex
	defer delete(neighbour_list)

	for edgeID in intVertex.edges {
		edge := intGraph.edges[edgeID]

		if edge.vertices[0] != id && edge.vertices[1] == id {
			append(&neighbour_list, intGraph.vertices[edge.vertices[0]])
		}

		if edge.vertices[0] == id && edge.vertices[1] != id {
			append(&neighbour_list, intGraph.vertices[edge.vertices[1]])
		}
	}

	for vertexIter in neighbour_list {
		DFS(vertexIter, intGraph, finished, visited)
	}

	append(&finished, intVertex)
}