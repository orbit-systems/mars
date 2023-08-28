package mars

import ph "./phobos" // frontend
import dm "./deimos" // backend
import mu "./utils"  // all mars utility functions
import "core:fmt"

// mars compiler core - executes and manages frontend + backend function

main :: proc() {
    intGraph := mu.createGraph()
    mu.addVertex(&intGraph)
    mu.addVertex(&intGraph)
    mu.addVertex(&intGraph)
    mu.addEdge((mu.edge){{0, 1}, .NON_DIRECTED, 0}, &intGraph)
    //mu.addEdge((mu.edge){{1, 2}, .NON_DIRECTED, 0}, &intGraph)
    //mu.addEdge((mu.edge){{0, 2}, .NON_DIRECTED, 0}, &intGraph)
    mu.hasCycle(intGraph)
}