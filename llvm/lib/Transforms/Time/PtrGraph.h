//===- PtrGraph.h - Representation of a graph of pointers -------*- C++ -*-===//
// It is oriented to implement the path profiling algorithm
//===----------------------------------------------------------------------===//

#ifndef PTRGRAPH_H_INCLUDED_
#define PTRGRAPH_H_INCLUDED_

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <vector>

class DirectedPtrGraphImpl {
public:
    template<typename PtrT> using AdjT     = std::set<PtrT>;
    template<typename PtrT> using EdgeT    = std::pair<PtrT, PtrT>;
    template<typename PtrT> using AdjListT = std::map<PtrT, AdjT<PtrT>>;

protected:
    AdjListT<void *>      Adjacencies;
    std::map<void *, int> Indegrees;

    DirectedPtrGraphImpl() = default;

    explicit DirectedPtrGraphImpl(const DirectedPtrGraphImpl &That) = default;

    explicit DirectedPtrGraphImpl(DirectedPtrGraphImpl &&That) = default;

    ~DirectedPtrGraphImpl() { }

    DirectedPtrGraphImpl &operator=(const DirectedPtrGraphImpl &) = default;

    DirectedPtrGraphImpl &operator=(DirectedPtrGraphImpl &&) = default;

public:
    size_t getNumNodes() const;

    size_t getNumEdges() const;

    bool contains(void * V) const;

    bool isEdge(void * V, void * W) const;

    bool insert(void * V);

    bool remove(void * V);

    bool connect(void * S, void * D);

    void disconnect(void * S, void * D);

    int indegree(void * V) const;

    int outdegree(void * V) const;

    std::vector<void *> getEntries() const {
        return getEntriesAs<void *>();
    }
    bool hasEntry() const {
        return !getEntries().empty();
    }

    bool hasSingleEntry() const {
        return getEntries().size() == 1;
    }

    void * getEntry() const {
        auto Es = getEntries();
        assert(Es.size() == 1 && "Has multiple entries");
        return Es[0];
    }

    std::vector<void *> getExits() const {
        return getExitsAs<void *>();
    }

    bool hasExit() const {
        return !getExits().empty();
    }

    bool hasSingleExit() const {
        return getExits().size() == 1;
    }

    void * getExit() const {
        auto Es = getExits();
        assert(Es.size() == 1 && "Has multiple entries");
        return Es[0];
    }

    const std::map<void *, int> & getAllIndegrees() const {
        return Indegrees;
    }

    std::vector<void *> getAllNodes() const {
        return getAllNodesAs<void *>();
    }

    std::vector<void *> getAdj(void * const V) const {
        return getAdjAs<void *>(V);
    }

    std::vector<EdgeT<void *>> getAllEdges() const {
        return getAllEdgesAs<void *>();
    }

    void print(std::ostream &OS) const;

    std::ostream & operator<<(std::ostream & OS) {
        print(OS);
        return OS;
    }

    void dump() const {
        print(std::cerr);
    }

protected:
    template<typename PtrT>
    static PtrT castFromVoidPtr(void * P) {
        return static_cast<PtrT>(P);
    }

    template<typename PtrT>
    static void * castToVoidPtr(PtrT P) {
        return static_cast<void *>(P);
    }

    template<typename PtrT>
    static PtrT getInvalidMarker() {
        return reinterpret_cast<PtrT>(-1);
    }

    template<typename PtrT>
    std::map<PtrT, int> getAllIndegreesAs() const {
        std::map<PtrT, int> Result;
        for (const auto & P : Indegrees) {
            Result[castFromVoidPtr<PtrT>(P.first)] = P.second;
        }
        return Result;
    }

    template<typename PtrT>
    std::vector<PtrT> getEntriesAs() const {
        std::vector<PtrT> Es;
        for (const auto & P : Adjacencies)
            if (indegree(P.first) == 0)
                // if this node has no incoming edge, it must be an entry
                Es.push_back(castFromVoidPtr<PtrT>(P.first));
        return Es;
    }

    template<typename PtrT>
    std::vector<PtrT> getExitsAs() const {
        std::vector<PtrT> Es;
        for (const auto & P : Adjacencies)
            if (outdegree(P.first) == 0)
                // if this node has no outgoing edge, it must be an exit
                Es.push_back(castFromVoidPtr<PtrT>(P.first));
        return Es;
    }

    template<typename PtrT>
    std::vector<PtrT> getAllNodesAs() const {
        std::vector<PtrT> Ns;
        for (auto &VW : Adjacencies)
            Ns.push_back(castFromVoidPtr<PtrT>(VW.first));
        return Ns;
    }

    template<typename PtrT>
    std::vector<PtrT> getAdjAs(const PtrT N) const {
        auto &Adj = Adjacencies.at(N);
        std::vector<PtrT> Result(Adj.size());
        std::transform(Adj.begin(), Adj.end(), Result.begin(), castFromVoidPtr<PtrT>);
        return Result;
    }

    template<typename PtrT>
    std::vector<EdgeT<PtrT>> getAllEdgesAs() const {
        std::vector<EdgeT<PtrT>> Es;
        for (auto &VW : Adjacencies)
            for (auto &W : VW.second)
                Es.push_back(
                        EdgeT<PtrT>(castFromVoidPtr<PtrT>(VW.first),
                                    castFromVoidPtr<PtrT>(W)));
        return Es;
    }
};

template<typename PtrType>
class DirectedPtrGraph : public DirectedPtrGraphImpl {
    // This is the common code among all the PtrGraph<>'s

public:
    using BaseT = DirectedPtrGraphImpl;
    using PtrT  = typename std::pointer_traits<PtrType>::pointer;

    DirectedPtrGraph() = default;

    DirectedPtrGraph(const DirectedPtrGraph &That) = default;

    DirectedPtrGraph(DirectedPtrGraph &&That) = default;

    ~DirectedPtrGraph() { }

    DirectedPtrGraph &operator=(const DirectedPtrGraph &) = default;

    DirectedPtrGraph &operator=(DirectedPtrGraph &&) = default;

    bool contains(PtrT V) const {
        return BaseT::contains(castToVoidPtr(V));
    }
    
    bool isEdge(PtrT V, PtrT W) const {
        return BaseT::isEdge(castToVoidPtr(V), castToVoidPtr(W));
    }

    bool insert(PtrT V) {
        return BaseT::insert(castToVoidPtr(V));
    }

    bool remove(PtrT V) {
        return BaseT::remove(castToVoidPtr(V));
    }

    bool connect(PtrT S, PtrT D) {
        return BaseT::connect(castToVoidPtr(S), castToVoidPtr(D));
    }

    void disconnect(PtrT S, PtrT D) {
        return BaseT::disconnect(castToVoidPtr(S), castToVoidPtr(D));
    }

    int indegree(PtrT V) const {
        return BaseT::indegree(castToVoidPtr(V));
    }

    int outdegree(PtrT V) const {
        return BaseT::outdegree(castToVoidPtr(V));
    }

    std::vector<PtrT> getEntries() const {
        return getEntriesAs<PtrT>();
    }

    PtrT getEntry() const {
        return castFromVoidPtr<PtrT>(BaseT::getEntry());
    }

    std::vector<PtrT> getExits() const {
        return getExitsAs<PtrT>();
    }

    PtrT getExit() const {
        return castFromVoidPtr<PtrT>(BaseT::getExit());
    }

    std::map<PtrT, int> getAllIndegrees() const {
        return getAllIndegreesAs<PtrT>();
    }

    std::vector<PtrT> getAllNodes() const {
        return getAllNodesAs<PtrT>();
    }

    std::vector<PtrT> getAdj(const PtrT N) const {
        return getAdjAs<PtrT>(N);
    }

    std::vector<EdgeT<PtrT>> getAllEdges() const {
        return getAllEdgesAs<PtrT>();
    }
};
#endif
