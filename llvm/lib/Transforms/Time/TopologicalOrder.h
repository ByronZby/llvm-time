//===- TopologicalOrder.h -  -------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef TOPOLOGICALORDER_H_INCLUDED_
#define TOPOLOGICALORDER_H_INCLUDED_

#include <list>

class DirectedPtrGraph;
class Vertex;

class TopologicalOrder {

    std::list<Vertex *> order;

public:
    TopologicalOrder() =delete;

    TopologicalOrder(const DirectedPtrGraph &);

    using iterator = decltype(order)::iterator;

    using reverse_iterator = decltype(order)::reverse_iterator;

    iterator begin() {
        return order.begin();
    }

    iterator end() {
        return order.end();
    }

    reverse_iterator rbegin() {
        return order.rbegin();
    }

    reverse_iterator rend() {
        return order.rend();
    }
};

#endif
