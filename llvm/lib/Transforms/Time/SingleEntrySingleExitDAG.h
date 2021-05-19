//===- SingleEntrySingleExitDAG.h -                      lgorithm - C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef SINGLEENTRYSINGLEEXITDAG_H_INCLUDED_
#define SINGLEENTRYSINGLEEXITDAG_H_INCLUDED_

#include "PtrGraph.h"

class SingleEntrySingleExitDAG : public DirectedPtrGraph {
public:
    SingleEntrySingleExitDAG() :
        NeedsUpdate(false), EntryVertex(nullptr), ExitVertex(nullptr) { }

    virtual ~SingleEntrySingleExitDAG() { }

    SingleEntrySingleExitDAG(const DirectedPtrGraph &that) :
        DirectedPtrGraph(that) {
        update();
    }

    SingleEntrySingleExitDAG(DirectedPtrGraph &&that) :
        DirectedPtrGraph(std::move(that)) {
        update();
    }

    SingleEntrySingleExitDAG(const SingleEntrySingleExitDAG &that) :
        DirectedPtrGraph(that),
        NeedsUpdate(that.NeedsUpdate) {
        EntryVertex = getVertex(that.EntryVertex->getValue<void*>());
        ExitVertex = getVertex(that.ExitVertex->getValue<void*>());
    }

    SingleEntrySingleExitDAG(SingleEntrySingleExitDAG && that) :
        DirectedPtrGraph(std::move(that)),
        NeedsUpdate(std::exchange(that.NeedsUpdate, false)),
        EntryVertex(std::exchange(that.EntryVertex, nullptr)),
        ExitVertex(std::exchange(that.ExitVertex, nullptr)) { }

    SingleEntrySingleExitDAG &operator =(const SingleEntrySingleExitDAG &rhs) =delete;

    template<typename T>
    T getExit() {
        if (NeedsUpdate) {
            update();
        }
        return ExitVertex->getValue<T>();
    }

    template<typename T>
    T getEntry() {
        if (NeedsUpdate) {
            update();
        }
        return EntryVertex->getValue<T>();
    }

    Vertex *getExitVertex() {
        if (NeedsUpdate) {
            update();
        }
        return ExitVertex;
    }

    Vertex *getEntryVertex() {
        if (NeedsUpdate) {
            update();
        }
        return EntryVertex;
    }

    void update();

protected:
    virtual void addVertexWithVertex(Vertex * const v) override {
        NeedsUpdate = true;
        DirectedPtrGraph::addVertexWithVertex(v);
    }

    virtual void addEdgeFromVertices(Vertex *src, Vertex *tgt) override {
        NeedsUpdate = true;
        DirectedPtrGraph::addEdgeFromVertices(src, tgt);
    }

    virtual void removeEdgeFromVertices(Vertex *src, Vertex *tgt) override {
        NeedsUpdate = true;
        DirectedPtrGraph::removeEdgeFromVertices(src, tgt);
    }

private:
    bool    NeedsUpdate;
    Vertex *EntryVertex;
    Vertex *ExitVertex;
};


#endif

