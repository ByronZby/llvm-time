
#include "PtrGraph.h"

size_t DirectedPtrGraphImpl::getNumNodes() const {
    return Adjacencies.size();
}

size_t DirectedPtrGraphImpl::getNumEdges() const {
    size_t Sum = 0;
    for (auto const &Pair : Adjacencies) {
        Sum += Pair.second.size();
    }
    return Sum;
}

bool DirectedPtrGraphImpl::contains(void * V) const {
    return Adjacencies.find(V) != Adjacencies.end();
}

bool DirectedPtrGraphImpl::isEdge(void * V, void * W) const {
    auto & Adjs = Adjacencies.at(V);
    return Adjs.find(W) != Adjs.end();
}

bool DirectedPtrGraphImpl::insert(void * V) {
    if (contains(V))
        return false;

    Adjacencies.emplace(V, AdjT<void *>());
    Indegrees[V] = 0;
    return true;
}

bool DirectedPtrGraphImpl::remove(void * V) {
    decltype(Adjacencies)::iterator Iter;
    if ((Iter = Adjacencies.find(V)) != Adjacencies.end()) {
        Adjacencies.erase(Iter);
        Indegrees.erase(Indegrees.find(V));

        for (auto &KV : Adjacencies) {
            KV.second.erase(V); //NOTE: No-op if V does not exist
        }
        return true;
    }

    return false;
}

bool DirectedPtrGraphImpl::connect(void * Src, void * Dest) {
    assert(contains(Src) && "connect(): Graph does not contain Src!");
    assert(contains(Dest) && "connect(): Graph does not contain Dest!");
    if (Adjacencies.at(Src).insert(Dest).second) {
        ++Indegrees[Dest];
        return true;
    }

    return false;
}

void DirectedPtrGraphImpl::disconnect(void * Src, void * Dest) {
    assert(contains(Src) && "disconnect(): Graph does not contain Src!");
    assert(contains(Dest) && "disconnect(): Graph does not contain Dest!");

    auto &Adj = Adjacencies.at(Src);
    assert(Adj.find(Dest) != Adj.end()
            && "disconnect(): Graph does not contain Src -> Dest!");

    Adj.erase(Dest);
    --Indegrees[Dest];
}

int DirectedPtrGraphImpl::indegree(void * V) const {
    return Indegrees.at(V);
}

int DirectedPtrGraphImpl::outdegree(void * V) const {
    return static_cast<int>(Adjacencies.at(V).size());
}

void DirectedPtrGraphImpl::print(std::ostream &OS) const {
    OS << "digraph {\n";
    for (const auto &VW : Adjacencies) {
        OS << "\t\"" << VW.first << "\" -> { ";
        for (const auto &W : VW.second) {
            OS << "\"" << W << "\" ";
        }
        OS << "};\n";
    }
    OS << "}";
}

