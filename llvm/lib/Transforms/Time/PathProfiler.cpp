//===- PathProfiler.h - An implementation of Ball-Larus algorithm - C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "PathProfiler.h"
#include "SingleEntrySingleExitDAG.h"
#include "TopologicalOrder.h"

#include <climits>
#include <iostream>
#include <map>
#include <queue>

using namespace std;

static void calculateEdgeValues(map<Edge, int> &Val,
                                const SingleEntrySingleExitDAG &G);

static map<Vertex *, Edge> getMST(const DirectedPtrGraph &G,
                                  const map<Edge, int> &Weights);

static void calculateEdgeIncrements(map<Edge, int> &Inc,
                                    const DirectedPtrGraph &G,
                                    const map<Edge, int> &Weights,
                                    const map<Vertex *, Edge> &MST);

PathProfiler::PathProfiler(SingleEntrySingleExitDAG &G) {
    void *Exit = G.getExit<void*>(), *Entry = G.getEntry<void*>();

    // Step 1: visit all vertices in reverse topological order to calculate
    // a unique value for each path and edge val corresponding to the paths
    calculateEdgeValues(Val, G);

    // Step 2: connect the exit vertex to the entry exit. This creates a
    // cycle and makes the graph no longer a DAG. Upcasting the graph to
    // a DirectedPtrGraph.
    Val[Edge(G.getExitVertex(), G.getEntryVertex())] = 0;

    DirectedPtrGraph &DG = static_cast<DirectedPtrGraph&>(G);

    DG.addEdge(Exit, Entry);

    // Step 3: Calculate the maximum spanning tree with edge values as
    // their weights
    map<Vertex *, Edge> MST = getMST(DG, Val);

    for (auto P : MST) {
        cerr << "MST: " << P.first->getValue<void*>() << " : " << P.second.src->getValue<void*>() << " -> "
             << P.second.dest->getValue<void*>() << endl;
    }

    // Step 4: Select edges to insert instrumentation on
    calculateEdgeIncrements(SelectedEdges, DG, Val, MST);

    // Restore SESEDAG
    DG.removeEdge(Exit, Entry);
    G.update();
}

static void calculateEdgeValues(map<Edge, int> &Val,
                                const SingleEntrySingleExitDAG &G) {
    map<Vertex *, int> NumPaths;
    TopologicalOrder Order(G);

    for (auto Iter = Order.rbegin(), End = Order.rend(); Iter != End; ++Iter) {
        Vertex *&V = *Iter;

        if (G.adj(V).empty()) {
            NumPaths[V] = 1;
        } else {
            NumPaths[V] = 0;
            for (const Edge &E : G.adj(V)) {
                Vertex *const W = E.dest;

                Val[E] = NumPaths[V];
                NumPaths[V] = NumPaths[V] + NumPaths[W];
            }
        }
    }

    for (auto Iter : Val) {
        const Edge &E = Iter.first;
        int &V = Iter.second;

        cerr << E.src->getValue<void*>() << " -> " << E.dest->getValue<void*>() << ": " << V << endl;
    }

    for (auto Iter : NumPaths) {
        cerr << "NumPaths " << Iter.first->getValue<void*>() << " : " << Iter.second << endl;
    }
}


static Vertex *maxNotIncludedVertex(const map<Vertex *, bool> &Included,
                                    const map<Vertex *, int> &Key) {
    int Max = -1;
    Vertex *MaxVertex = nullptr;

    for (auto P : Key) {
        if (!Included.at(P.first) && P.second > Max) {
            Max = P.second;
            MaxVertex = P.first;
        }
    }

    assert(MaxVertex != nullptr);
    return MaxVertex;
}

static map<Vertex *, Edge> getMST(const DirectedPtrGraph &G,
                                  const map<Edge, int> &Weights) {
    map<Vertex *, Edge> MST;
    map<Vertex *, bool> Included;
    map<Vertex *, int> DistToMST;

    list<Vertex *> allVertices = G.allVertices();

    for (Vertex *const V : allVertices) {
        DistToMST[V] = INT_MIN;
        Included[V] = false;
    }

    Vertex *const &FirstVertex = allVertices.front();
    DistToMST[FirstVertex] = 1; // it'll be picked since all others are 0

    for (size_t Count = 0, Total = G.V() - 1; Count < Total; Count++) {
        Vertex *V = maxNotIncludedVertex(Included, DistToMST);

        Included[V] = true;

        for (Vertex *const W : allVertices) {
            Edge E;
            if (G.isEdge(V, W)) {
                E = {V, W};
            } else if (G.isEdge(W, V)) {
                E = {W, V};
            } else {
                continue;
            }

            if (!Included[W] && Weights.at(E) > DistToMST[W]) {
                DistToMST[W] = Weights.at(E);
                MST[W] = Edge(W, V);
            }
        }
    }

    assert(MST.size() == G.V() - 1 && "MST() size != V - 1 !");

    return MST;
}

static inline bool edgeInMST(Vertex *const V,
                             Vertex *const W,
                             const map<Vertex *, Edge> &MST) {
    map<Vertex *, Edge>::const_iterator Iter;
    return ((Iter = MST.find(V)) != MST.end() && Iter->second.dest == W) ||
           ((Iter = MST.find(W)) != MST.end() && Iter->second.dest == V);
}

static inline list<Edge> pathInMST(Vertex *const Src,
                                   Vertex *const Dest,
                                   const map<Vertex *, Edge> &MST) {
    if (Src == Dest) {
        return list<Edge>();
    }

    queue<Vertex *> Q;
    set<Vertex *> Visited;
    map<Vertex *, Vertex *> Pred;

    Visited.insert(Src);
    Q.push(Src);

    while (!Q.empty()) {
        Vertex *V = Q.front();
        Q.pop();

        for (const auto &P : MST) {
            auto &E = P.second;
            Vertex *W;
            if (E.src == V) {
                W = E.dest;
            } else if (E.dest == V) {
                W = E.src;
            } else {
                continue;
            }

            if (Visited.find(W) == Visited.end()) {
                Q.push(W);
                Visited.insert(W);
                Pred[W] = V;

                if (W == Dest) {
                    goto found;
                }
            }
        }
    }

    assert(0 && "pathInMST() dest is not reachable from src!");

found:
    list<Edge> Path;
    Vertex *W = Dest;
    do {
        Vertex *V = Pred[W];
        Path.push_front(Edge(V, W));
        W = V;
    } while (W != Src);

    return Path;
}


static void calculateEdgeIncrements(map<Edge, int> &Inc,
                                    const DirectedPtrGraph &G,
                                    const map<Edge, int> &Weights,
                                    const map<Vertex *, Edge> &MST) {

    for (const Edge &Chord : G.allEdges()) {
        Vertex *const V = Chord.src;
        Vertex *const W = Chord.dest;
        if (!edgeInMST(V, W, MST)) {
            cerr << "PATH: Getting path " << V->getValue<void*>() << " -> " << W->getValue<void*>() << endl;
            list<Edge> Path = pathInMST(V, W, MST);
            int CurrInc = 0;
            for (auto &E : Path) {
                cerr << "PATH: " << E.src->getValue<void*>() << " -> " << E.dest->getValue<void*>() << endl;
                if (G.isEdge(E.src, E.dest)) {
                    CurrInc -= Weights.at(E);
                } else if (G.isEdge(E.dest, E.src)) {
                    CurrInc += Weights.at(Edge(E.dest, E.src));
                } else {
                    assert(0 && "EdgeIncrements() impossible: path edge is not in the graph!");
                }
            }
            CurrInc += Weights.at(Chord);
            cerr << "PATH: increment is " << CurrInc << endl;
            if (CurrInc != 0) {
                Inc[Chord] = CurrInc;
            }
        }
    }
}

