//===- PtrGraph.cpp - Implementation of a graph of pointers -----*- C++ -*-===//
// It is oriented to implement the path profiling algorithm
//===----------------------------------------------------------------------===//

#include "PtrGraph.h"

#include <iostream>
#include <iomanip>

using namespace std;

ostream &Vertex::operator<<(ostream &os) {
    os << "<Vertex> payload: " << val << " tag: " << tag;
    return os;
}

ostream &PtrGraphBase::operator<<(ostream &os) {
    os << "<Graph with " << vertices.size() << " vertices>";
    return os;
}

void PtrGraphBase::print(ostream &os) const {
    os << "digraph {\n";
    for (const auto &iter : adjacencies) {
        os << "\t\"" << iter.first->getValue<void*>() << "\" -> { ";
        for (const auto &edge : iter.second) {
            os << "\"" << edge.dest->getValue<void*>() << "\" ";
        }
        os << "};\n";
    }
    os << "}";
}

void PtrGraphBase::invariant() const {
    // 1. In vertices map, key and value correspond
    for (const auto iter : vertices) {
        assert(iter.first == iter.second->getValue<void*>() && "Invariant 1!");
    }

    // 2. All vertices have adjacency list
    for (const auto iter : vertices) {
        assert(adjacencies.find(iter.second) != adjacencies.end() && "Invariant 3!");
    }

    // 3. All edges are within the graph
    for (const auto iter : adjacencies) {
        for (const auto edge : iter.second) {
            assert(contains(edge.src->getValue<void*>()) && "Invariant 3!");
            assert(contains(edge.dest->getValue<void*>()) && "Invariant 3!");
        }
    }
}

void PtrGraphBase::copyTo(PtrGraphBase &that) const {
    map<void *, Vertex *> vs;
    map<Vertex *, std::set<Edge>> adjs;

    // allocate and copy all vertices
    for (const auto &iter : vertices) {
        vs[iter.first] = new Vertex(*iter.second);
    }

    // for all vertices, copy the adjacencies
    for (const auto &iter : vertices) {
        auto v = vs[iter.first];
        auto &edges = adjs[v];

        for (const auto &edge : adjacencies.at(iter.second)) {
            auto w = vs[edge.dest->getValue<void*>()];

            edges.insert(Edge(v, w));
        }
    }

    that.vertices = move(vs);
    that.adjacencies = move(adjs);
}



