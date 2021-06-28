
#include "GraphAlgorithms.h"

#include "PtrGraph.h"

#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <queue>

using namespace std;

CycleImpl::CycleImpl(const DirectedPtrGraphImpl &G) {
    map<void *, int> Indegrees = G.getAllIndegrees();
    queue<void *> Queue;

    for (void * const V : G.getAllNodes())
        if (Indegrees[V] == 0)
            Queue.push(V);

    while (!Queue.empty()) {
        void * V = Queue.front();
        Queue.pop();

        for (void * const &W : G.getAdj(V)) {
            --Indegrees[W];

            if (Indegrees[W] == 0)
                Queue.push(W);
        }
    }

    // if there are any vertices whose indegree is not 0,
    // there is a cycle
    map<void *, void *> Path;
    void * Root = nullptr;
    for (void * const V : G.getAllNodes()) {
        if (Indegrees[V] > 0) {
            Root = V;

            for (void * const W : G.getAdj(V)) 
                if (Indegrees[W] > 0)
                    Path[W] = V;
        }
    }

    if (Root != nullptr) {
        set<void *> Visited;
        while (Visited.find(Root) == Visited.end()) {
            Visited.insert(Root);
            Root = Path[Root];
        }

        void * V = Root;
        do {
            Nodes.push_back(V);
            V = Path[V];
        } while (V != Root);

        Nodes.push_back(Root);
    }
}

TopologicalOrderImpl::TopologicalOrderImpl(const DirectedPtrGraphImpl &G) {
    map<void *, int> Indegrees = G.getAllIndegrees();
    queue<void *> Queue;

    for (void * const V : G.getAllNodes())
        if (Indegrees[V] == 0)
            Queue.push(V);

    size_t Count = 0;

    while (!Queue.empty()) {
        void * V = Queue.front();
        Queue.pop();
        ++Count;

        Order.push_back(V);

        for (void * const W : G.getAdj(V)) {
            --Indegrees[W];
            if (Indegrees[W] == 0)
                Queue.push(W);
        }
    }

    if (Count != G.getNumNodes())
        Order.clear();
}


void PathProfilerImpl::calculateEdgeValues() {
    map<void *, int>     NumPaths;
    TopologicalOrderImpl Order(G);

    for (auto Iter = Order.rbegin(), End = Order.rend(); Iter != End; ++Iter) {
        // In reverse topological order
        void * V = *Iter;

        if (G.getAdj(V).empty()) {
            NumPaths[V] = 1;
        } else {
            NumPaths[V] = 0;

            for (void * W : G.getAdj(V)) {
                Val[make_pair(V, W)] = NumPaths[V];
                NumPaths[V] = NumPaths[V] + NumPaths[W];
            }
        }
    }

#ifndef NDEBUG
    for (const auto & P : Val) {
        const auto & E = P.first;
        const int    V = P.second;

        cerr << "Val " << E.first << " -> " << E.second << " : " << V << endl;
    }

    for (const auto & P : NumPaths) {
        cerr << "NumPaths " << P.first << " : " << P.second << endl;
    }
#endif
}

PathProfilerImpl::PathProfilerImpl(const DirectedPtrGraphImpl & Graph)
    : G(Graph) {
    calculateEdgeValues();
}

