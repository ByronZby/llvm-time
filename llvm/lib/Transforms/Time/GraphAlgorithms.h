//===- PtrGraph.h - Representation of a graph of pointers -------*- C++ -*-===//
// It is oriented to implement the path profiling algorithm
//===----------------------------------------------------------------------===//

#ifndef GRAPHALGORITHMS_H_INCLUDED_
#define GRAPHALGORITHMS_H_INCLUDED_

#include "GraphUtils.h"

#include <algorithm>
#include <deque>
#include <tuple>
#include <map>
#include <vector>

class DirectedPtrGraphImpl;
template <typename PtrT> class DirectedPtrGraph;

class CycleImpl {
protected:
    std::vector<void *> Nodes;

public:
    CycleImpl(const DirectedPtrGraphImpl &);

    auto begin() {
        return Nodes.begin();
    }

    auto end() {
        return Nodes.end();
    }

    auto rbegin() {
        return Nodes.rbegin();
    }

    auto rend() {
        return Nodes.rend();
    }
};

template<typename PtrT>
class Cycle : CycleImpl {

public:
    Cycle() = delete;

    Cycle(const DirectedPtrGraph<PtrT> &DPG) : CycleImpl(DPG) { }

    Cycle(const Cycle &) = default; 

    Cycle(Cycle &&) = default;

    ~Cycle() { }

    bool hasCycle() const {
        return !Nodes.empty();
    }

    bool inCycle(PtrT N) const {
        return std::find(Nodes.begin(), Nodes.end(), N) != Nodes.end();
    }

    using iterator = PointerCastIterator<PtrT, decltype(Nodes)::iterator>;

    using reverse_iterator = PointerCastIterator<PtrT, decltype(Nodes)::reverse_iterator>;

    iterator begin() {
        return iterator(Nodes.begin());
    }

    iterator end() {
        return iterator(Nodes.end());
    }

    reverse_iterator rbegin() {
        return reverse_iterator(Nodes.rbegin());
    }

    reverse_iterator rend() {
        return reverse_iterator(Nodes.rend());
    }
};

class TopologicalOrderImpl {

protected:
    std::deque<void *> Order;

public:
    TopologicalOrderImpl(const DirectedPtrGraphImpl &);

    auto begin() {
        return Order.begin();
    }

    auto end() {
        return Order.end();
    }

    auto rbegin() {
        return Order.rbegin();
    }

    auto rend() {
        return Order.rend();
    }
};

template <typename PtrT>
class TopologicalOrder : TopologicalOrderImpl {

public:

    TopologicalOrder() = delete;

    TopologicalOrder(const DirectedPtrGraph<PtrT> &G) :
        TopologicalOrderImpl(G) { }

    TopologicalOrder(const TopologicalOrder &) = default;

    TopologicalOrder(TopologicalOrder &&) = default;

    using iterator = PointerCastIterator<PtrT, decltype(Order)::iterator>;

    using reverse_iterator = PointerCastIterator<PtrT, decltype(Order)::reverse_iterator>;

    iterator begin() {
        return iterator(Order.begin());
    }

    iterator end() {
        return iterator(Order.end());
    }

    reverse_iterator rbegin() {
        return reverse_iterator(Order.rbegin());
    }

    reverse_iterator rend() {
        return reverse_iterator(Order.rend());
    }
};


class PathProfilerImpl {
public:
    using EdgeT = std::pair<void *, void *>;

protected:
    std::map<EdgeT, int>         Val;
    const DirectedPtrGraphImpl & G;

    PathProfilerImpl(const DirectedPtrGraphImpl &);

private:
    void calculateEdgeValues();
};

template <typename PtrT>
class PathProfiler : public PathProfilerImpl {

    static constexpr std::pair<std::pair<PtrT, PtrT>, int>
    castEdge(const std::pair<EdgeT, int> & P) {
        return std::make_pair(
                std::make_pair(static_cast<PtrT>(P.first.first),
                               static_cast<PtrT>(P.first.second)),
                P.second);
    }

public:
    PathProfiler() = delete;

    PathProfiler(const DirectedPtrGraphImpl & G) :
        PathProfilerImpl(G) { }

    using iterator = CastIterator<decltype(castEdge), castEdge,
                                  std::map<EdgeT, int>::iterator>;

    iterator begin() {
        return iterator(Val.begin());
    }

    iterator end() {
        return iterator(Val.end());
    }
};

template<typename T>
inline Cycle<T> getCycle(const DirectedPtrGraph<T> & G) {
    return Cycle<T>(G);
}

template<typename T>
inline TopologicalOrder<T> getTopologicalOrder(const DirectedPtrGraph<T> & G) {
    return TopologicalOrder<T>(G);
}

template<typename T>
inline PathProfiler<T> getPathProfiler(const DirectedPtrGraph<T> & G) {
    return PathProfiler<T>(G);
}

#endif
