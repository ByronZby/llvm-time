//===- SingleEntrySingleExitDAG.h -                      lgorithm - C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "SingleEntrySingleExitDAG.h"
#include "Cycle.h"

#include <tuple>

#define SESEDAG SingleEntrySingleExitDAG

static inline bool hasCycle(const SESEDAG &G) {
    Cycle C(G);
    return C.hasCycle();
}

static inline std::pair<Vertex *, Vertex *> getEntryAndExit(const SESEDAG &G) {
    Vertex *Entry = nullptr;
    Vertex *Exit = nullptr;
    for (Vertex * const V : G.allVertices()) {
        if (G.indegree(V) == 0) {
            assert(!Entry && "SESEDAG() Graph has more than one entry!");
            Entry = V;
        }

        if (G.adj(V).empty()) {
            assert(!Exit && "SESEDAG() Graph has more than one exit!");
            Exit = V;
        }
    }
    return std::make_pair(Entry, Exit);
}

void SESEDAG::update() {
    assert(!hasCycle(*this) && "update() Graph is not a DAG!");
    std::tie(EntryVertex, ExitVertex) = getEntryAndExit(*this);
    NeedsUpdate = false;
}


