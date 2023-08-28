package mars_utils

import "core:fmt"
import "core:slice"

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

removeEdge :: proc(intGraph: ^graph, id: int) {
	unordered_remove(&intGraph.edges, id)
}

addVertex :: proc(intGraph: ^graph) {
	intVertex := new(vertex)
	append(&intGraph.vertices, intVertex)
}

edge_sort_proc :: proc(a: edge, b: edge) -> bool {
	if a.weight < b.weight {
		return true
	}
	return false
}

MST :: proc(intGraph: graph) -> graph {
	//using kruskal's algorithm
	candidateEdges: [dynamic]edge 
	defer delete(candidateEdges)

	if (len(intGraph.edges) - 1) < (len(intGraph.vertices) - 1) {
		return (graph){}
	}

	append(&candidateEdges, ..intGraph.edges[:])
	slice.sort_by(candidateEdges[:], edge_sort_proc)
	newGraph := createGraph()
	for vertex in intGraph.vertices {
		addVertex(&newGraph)
	}

	for i:=0; i < len(intGraph.vertices) - 1; i+=1 {
		addEdge(candidateEdges[i], &newGraph)
		santizeGraph(&newGraph)
		fmt.printf("Testing edge {}, connections {}<->{}, weight {}\n", i, candidateEdges[i].vertices[0], candidateEdges[i].vertices[1], candidateEdges[i].weight)

		if hasCycle(newGraph) == true {
			ordered_remove(&candidateEdges, i)
			slice.sort_by(candidateEdges[:], edge_sort_proc)
			clear(&newGraph.edges)
			i = -1
			if (len(candidateEdges) + 1) == len(intGraph.vertices) {
				pop(&candidateEdges)
				append(&newGraph.edges, ..candidateEdges[:])
				santizeGraph(&newGraph)
				break
			}
		}
	}

	return newGraph
} 

@(private) cycleDetected: bool

hasCycle :: proc(intGraph: graph) -> bool {
	finished, visited: [dynamic]^vertex
	defer delete(finished)
	defer delete(visited)

	cycleDetected = false

	for vertex, i in intGraph.vertices {
		DFS(vertex, intGraph, finished, visited, i)
	}

	return cycleDetected
}

DFS :: proc(intVertex: ^vertex, intGraph: graph, finished, visited: [dynamic]^vertex, caller_id: int) {
	finished := finished
	visited  := visited

	if cycleDetected == true {
		return
	}

	//get vertex id
	id : int = ---
	for vertexIter, i in intGraph.vertices {
		if vertexIter == intVertex {
			id = i
		}
	}
	//fmt.printf("Traversing id {}!\n", id)

	for vertexIter in finished { //if finished(v)
		if vertexIter == intVertex {
			return
		}
		//fmt.printf("Finished on id {}!\n", id)
	}

	for vertexIter in visited { //if visited(v)
		if vertexIter == intVertex {
			fmt.printf("Found cycle on id: {}!\n", id)
			cycleDetected = true
			return
		}
	}

	append(&visited, intVertex)
	neighbour_list: [dynamic]^vertex
	defer delete(neighbour_list)

	for edgeID in intVertex.edges {
		edge := intGraph.edges[edgeID]

		if edge.vertices[0] != id && edge.vertices[1] == id && edge.vertices[0] != caller_id {
			append(&neighbour_list, intGraph.vertices[edge.vertices[0]])
		}

		if edge.vertices[0] == id && edge.vertices[1] != id && edge.vertices[1] != caller_id{
			append(&neighbour_list, intGraph.vertices[edge.vertices[1]])
		}
	}

	for vertexIter in neighbour_list {
		DFS(vertexIter, intGraph, finished, visited, id)
	}

	append(&finished, intVertex)
}