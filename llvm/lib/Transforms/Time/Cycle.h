//===- Cycle.h - Finding cycles in a Graph ----------------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef CYCLE_H_INCLUDED_
#define CYCLE_H_INCLUDED_

#include <list>
#include <algorithm>

class DirectedPtrGraph;
class Vertex;

class Cycle {

    std::list<Vertex *> cycle;

public:
    Cycle() =delete;

    Cycle(const DirectedPtrGraph &);

    bool hasCycle() const {
        return !cycle.empty();
    }

    bool inCycle(Vertex *V) const {
        return std::find(cycle.begin(), cycle.end(), V) != cycle.end();
    }
    
    using iterator = decltype(cycle)::iterator;

    using reverse_iterator = decltype(cycle)::reverse_iterator;

    iterator begin() {
        return cycle.begin();
    }

    iterator end() {
        return cycle.end();
    }

    reverse_iterator rbegin() {
        return cycle.rbegin();
    }

    reverse_iterator rend() {
        return cycle.rend();
    }

};


#endif

