//===- GraphGenerator.cpp - Generate random graphs -------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "GraphGenerator.h"
#include "PtrGraph.h"

#include <set>
#include <vector>
#include <type_traits>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

#define SEED() srand(time(nullptr))

namespace {

int COUNT = 0;

inline void newnames() {
    COUNT++;
}

inline void *ptr(int i) {
    return reinterpret_cast<void *>((COUNT << 16) | i);
}

template<typename Graph>
inline void simpleTemplate(Graph &G, int V, int E) {
    static_assert(is_base_of<PtrGraphBase, Graph>::value && "simpleTemplate() template not a graph!");

    assert(V > 0 && "simple() V > 0!");
    assert(E <= static_cast<long>(V) * (V - 1) / 2 && "simple() too many edges!");

    SEED();
    newnames();

    for (int i = 0; i < V; i++) {
        G.addVertex(ptr(i));
    }

    set<Edge> S;
    while (G.E() < static_cast<size_t>(E)) {
        void *v = ptr(rand() % V);
        void *w = ptr(rand() % V);

        Edge e(G.getVertex(v), G.getVertex(w));
        if (v != w && S.find(e) == S.end()) {
            S.insert(e);
            G.addEdge(v, w);
        }
    }
}

template<typename Graph>
inline void pathTemplate(Graph &G, int V) {
    static_assert(is_base_of<PtrGraphBase, Graph>::value && "pathTemplate() template not a graph!");

    SEED();
    newnames();

    vector<void *> vs;
    for (int i = 0; i < V; i++) {
        vs.push_back(ptr(i));
        G.addVertex(ptr(i));
    }

    random_shuffle(vs.begin(), vs.end());

    for (int i = 0; i < V - 1; i++) {
        G.addEdge(vs[i], vs[i + 1]);
    }
}

template<typename Graph>
inline void cycleTemplate(Graph &G, int V) {
    static_assert(is_base_of<PtrGraphBase, Graph>::value && "pathTemplate() template not a graph!");

    SEED();
    newnames();

    vector<void *> vs;
    for (int i = 0; i < V; i++) {
        vs.push_back(ptr(i));
        G.addVertex(ptr(i));
    }

    random_shuffle(vs.begin(), vs.end());

    for (int i = 0; i < V - 1; i++) {
        G.addEdge(vs[i], vs[i + 1]);
    }

    G.addEdge(vs[V - 1], vs[0]);
}


} // anonymous namespace

void GraphGenerator::simple(UndirectedPtrGraph &g, int V, int E) {
    simpleTemplate(g, V, E);
}

void GraphGenerator::simple(DirectedPtrGraph &g, int V, int E) {
    simpleTemplate(g, V, E);
}

void GraphGenerator::path(UndirectedPtrGraph &g, int V) {
    pathTemplate(g, V);
}

void GraphGenerator::path(DirectedPtrGraph &g, int V) {
    pathTemplate(g, V);
}

void GraphGenerator::cycle(UndirectedPtrGraph &g, int V) {
    cycleTemplate(g, V);
}

void GraphGenerator::cycle(DirectedPtrGraph &g, int V) {
    cycleTemplate(g, V);
}

void GraphGenerator::DAG(DirectedPtrGraph &G, int V, int E) {
    assert(V > 0 && "DAG() V <= 0!");
    assert(E <= static_cast<long>(V) * (V - 1) / 2 && "DAG() too many edges!");

    SEED();
    newnames();

    vector<void *> vs;
    for (int i = 0; i < V; i++) {
        vs.push_back(ptr(i));
        G.addVertex(ptr(i));
    }

    random_shuffle(vs.begin(), vs.end());

    set<pair<void *, void *>> S;
    while (G.E() < static_cast<size_t>(E)) {
        int v = rand() % V;
        int w = rand() % V;
        
        auto p = make_pair(ptr(v), ptr(w));
        if (v < w && S.find(p) == S.end()) {
            S.insert(p);
            G.addEdge(vs[v], vs[w]);
        }
    }
}

void GraphGenerator::naturalLoop(DirectedPtrGraph &G, int V, int E) {
    //TODO: IT IS NOT A NATURAL LOOP
    assert(V > 0 && "naturalLoop() V <= 0!");
    assert(E <= static_cast<long>(V) * (V - 1) / 2 + 1 && "naturalLoop() too many edges!");

    SEED();
    newnames();

    vector<void *> vs;
    for (int i = 0; i < V; i++) {
        vs.push_back(ptr(i));
        G.addVertex(ptr(i));
    }

    random_shuffle(vs.begin(), vs.end());

    set<pair<void *, void *>> S;
    while (G.E() < (size_t)E - 1) {
        int v = rand() % V;
        int w = rand() % V;
        
        auto p = make_pair(ptr(v), ptr(w));
        if (v < w && S.find(p) == S.end()) {
            S.insert(p);
            G.addEdge(vs[v], vs[w]);
        }
    }

    int backedge_v = rand() % (V - 1) + 1;

    G.addEdge(vs[backedge_v], vs[0]);
}


