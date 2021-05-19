//===- PathProfiler.h - An implementation of Ball-Larus algorithm - C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef PATHPROFILER_H_INCLUDED_
#define PATHPROFILER_H_INCLUDED_

#include "PtrGraph.h"

#include <map>

class SingleEntrySingleExitDAG;

class PathProfiler {
    std::map<Edge, int> SelectedEdges;
    std::map<Edge, int> Val;

public:
    PathProfiler() =delete;

    PathProfiler(SingleEntrySingleExitDAG &G);

    using iterator = decltype(SelectedEdges)::iterator;

    iterator begin() {
        return SelectedEdges.begin();
    }

    iterator end() {
        return SelectedEdges.end();
    }

    int getEdgeVal(Vertex *S, Vertex *D) const {
        return Val.at(Edge(S, D));
    }
};

#endif

