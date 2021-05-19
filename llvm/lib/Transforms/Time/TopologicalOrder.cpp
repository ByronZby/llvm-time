//===- TopologicalOrder.h -  -------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "TopologicalOrder.h"
#include "PtrGraph.h"

#include <map>
#include <queue>
#include <utility>

using namespace std;

TopologicalOrder::TopologicalOrder(const DirectedPtrGraph &G) {
    // get all indegrees
    map<Vertex *, int> indegrees;

    for (Vertex * const &v : G.allVertices()) {
        indegrees[v] = G.indegree(v);
    }

    queue<Vertex *> queue;

    for (Vertex * const &v : G.allVertices()) {
        if (indegrees[v] == 0) {
            queue.push(v);
        }
    }

    size_t count = 0;

    while (!queue.empty()) {
        Vertex *v = queue.front();
        queue.pop();
        count++;

        order.push_back(v);

        for (const Edge &e : G.adj(v)) {
            indegrees[e.dest]--;
            if (indegrees[e.dest] == 0) {
                queue.push(e.dest);
            }
        }
    }

    if (count != G.V()) {
        // has cycle
        order.clear();
    }
}

