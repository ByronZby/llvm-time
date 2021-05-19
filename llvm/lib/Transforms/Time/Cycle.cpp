//===- Cycle.cpp - Find a cycle in graph  -------*- C++ -*-===//
// The implementation is based on the same algorithm of finding a topological
// order in a DAG.
//===----------------------------------------------------------------------===//

#include "Cycle.h"
#include "PtrGraph.h"

#include <queue>
#include <set>
#include <map>

#include <iostream>

using namespace std;

Cycle::Cycle(const DirectedPtrGraph &G) : cycle() {
    map<Vertex *, int> indegrees;
    for (Vertex * const &v : G.allVertices()) {
        indegrees[v] = G.indegree(v);
    }

    queue<Vertex *> queue;

    for (Vertex * const &v : G.allVertices()) {
        if (indegrees[v] == 0)
            queue.push(v);
    }

    while (!queue.empty()) {
        Vertex *v = queue.front();
        queue.pop();

        for (const Edge &e : G.adj(v)) {
            indegrees[e.dest]--;

            if (indegrees[e.dest] == 0) 
                queue.push(e.dest);
        }
    }

    // if there are any vertices whose indegree is not 0,
    // there is a cycle
    map<Vertex *, Vertex *> path;
    Vertex *root = nullptr;
    for (Vertex * const &v : G.allVertices()) {
        if (indegrees[v] > 0) {
            root = v;

            for (const Edge &e : G.adj(v)) {
                if (indegrees[e.dest] > 0) {
                    path[e.dest] = v;
                }
            }
        }
    }

    if (root != nullptr) {
        set<Vertex *> visited;
        while (visited.find(root) == visited.end()) {
            visited.insert(root);
            root = path[root];
        }

        Vertex *v = root;
        do {
            cycle.push_front(v);
            v = path[v];
        } while (v != root);

        cycle.push_front(root);
    }
}


