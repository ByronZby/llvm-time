//===- PtrGraph.h - Representation of a graph of pointers -------*- C++ -*-===//
// It is oriented to implement the path profiling algorithm
//===----------------------------------------------------------------------===//

#ifndef PTRGRAPH_H_INCLUDED_
#define PTRGRAPH_H_INCLUDED_

#include "Exchange.h"

#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <utility>

class Vertex {
public:
    Vertex() : Vertex(nullptr) { }

    Vertex(void *ptr) : val(ptr), tag(false) { }

    Vertex(const Vertex &that) : val(that.val), tag(that.tag) { }

    Vertex(Vertex &&that) : 
        val(std::exchange(that.val, nullptr)),
        tag(std::exchange(that.tag, false)) { }

    Vertex &operator=(const Vertex &that) {
        val = that.val;
        tag = that.tag;

        return *this;
    }

    ~Vertex() { /*TODO: deregister*/ }

    template<typename T>
    T getValue() const {
        static_assert(std::is_pointer<T>(), "getValue() needs a Pointer");
        return static_cast<T>(val);
    }

    std::ostream &operator<<(std::ostream &os);

    bool operator==(const Vertex &rhs) const {
        return val == rhs.val;
    }

    bool operator!=(const Vertex &rhs) const {
        return !(*this == rhs);
    }

    bool operator< (const Vertex &rhs) const {
        return val < rhs.val;
    }

    bool operator> (const Vertex &rhs) const {
        return rhs < *this;
    }

    bool operator<=(const Vertex &rhs) const {
        return !(*this > rhs);
    }

    bool operator>=(const Vertex &rhs) const {
        return !(*this < rhs);
    }

    int setTag(int v) {
        int old = tag;
        tag = v;
        return old;
    }

    int getTag() const {
        return tag;
    }

private:
    void *val;
    int tag;
};

struct Edge {
    Vertex *src;
    Vertex *dest;
    int weight;

    Edge() : Edge(nullptr, nullptr) { }
    Edge(Vertex *s, Vertex *d) : Edge(s, d, 0) { }
    Edge(Vertex *s, Vertex *d, int w) : src(s), dest(d), weight(w) { }

    bool operator==(const Edge &rhs) const {
        return src == rhs.src && dest == rhs.dest;
    }

    bool operator!=(const Edge &rhs) const {
        return !(*this == rhs);
    }

    bool operator< (const Edge &rhs) const {
        if (src == rhs.src) {
            return dest < rhs.dest;
        }
        return src < rhs.src;
    }

    bool operator> (const Edge &rhs) const {
        return rhs < *this;
    }

    bool operator<=(const Edge &rhs) const {
        return !(*this > rhs);
    }

    bool operator>=(const Edge &rhs) const {
        return !(*this < rhs);
    }
};

class PtrGraphBase {
public:
    PtrGraphBase(const PtrGraphBase &that) : PtrGraphBase() {
        that.copyTo(*this);
    }

    PtrGraphBase(PtrGraphBase &&that) : vertices(std::move(that.vertices)), adjacencies(std::move(that.adjacencies)) { }

    virtual ~PtrGraphBase() {
        for (auto &v : vertices) {
            delete v.second;
        }
    }

    size_t V() const {
        return vertices.size();
    }

    size_t E() const {
        size_t sum = 0;
        for (const auto &iter : adjacencies) {
            sum += iter.second.size();
        }
        return sum;
    }

    template<typename T>
    void addVertex(T ptr) noexcept {
        static_assert(std::is_pointer<T>(), "addVertex() needs a pointer!");
        assert(!contains(ptr) && "Vertex already exists in graph!");

        auto *v = new Vertex(ptr);
        addVertexWithVertex(v);
    }

    template<typename T>
    void removeVertex(T ptr) noexcept {
        static_assert(std::is_pointer<T>(), "removeVertex() needs a pointer!");
        auto result = vertices.find(static_cast<void *>(ptr));
        if (result != vertices.end()) {
            adjacencies.erase(result->second);
            delete result->second;
            vertices.erase(result);
        }
    }

    template<typename T>
    Vertex *getVertex(T ptr) const {
        static_assert(std::is_pointer<T>(), "removeVertex() needs a pointer!");
        assert(contains(ptr) && "removeVertex() ptr does not exist!");

        return vertices.at(static_cast<void *>(ptr));
    }

    std::list<Vertex *> allVertices() const {
        std::list<Vertex *> vs;
        for (auto &v : vertices) {
            vs.push_back(v.second);
        }

        return vs;
    }

    template<typename T>
    void addEdge(T src, T tgt) {
        static_assert(std::is_pointer<T>(), "addEdge() needs pointers!");
        Vertex *v = getVertex(src);
        Vertex *w = getVertex(tgt);

        addEdgeFromVertices(v, w);
    }

    template<typename T>
    void removeEdge(T src, T tgt) {
        static_assert(std::is_pointer<T>(), "addEdge() needs pointers!");
        Vertex *v = getVertex(src);
        Vertex *w = getVertex(tgt);

        removeEdgeFromVertices(v, w);
    }

    bool isEdge(Vertex *const v, Vertex *const w) const {
        auto & es = adjacencies.at(v);
        return es.find(Edge(v, w)) != es.end();
    }

    template<typename T>
    bool contains(T ptr) const noexcept {
        static_assert(std::is_pointer<T>(), "contains() needs a pointer!");
        return vertices.find(static_cast<void *>(ptr)) != vertices.end();
    }

    /*
    template<typename T>
    const std::set<Edge> &adj(T src) const {
        assert(contains(src) && "getEdgesFrom() vertex does not exist!");
        void *s = static_cast<void *>(src);
        return adjacencies.at(vertices.at(s));
    }
    */

    const std::set<Edge> adj(Vertex * const v) const {
        return adjacencies.at(v);
    }

    const std::list<Edge> allEdges() const {
        std::list<Edge> es;
        for (const auto &p : adjacencies) {
            for (const auto &e : p.second) {
                es.push_back(e);
            }
        }
        return es;
    }

    void copyTo(PtrGraphBase &that) const;

    void print(std::ostream &os) const;

    PtrGraphBase &operator=(const PtrGraphBase &that) {
        that.copyTo(*this);
        return *this;
    }

    std::ostream &operator<<(std::ostream &os);

    void invariant() const;

protected:
    PtrGraphBase() : vertices(), adjacencies() { }

    void addVertex(const Vertex &vertex) {
        auto *v = new Vertex(vertex);
        addVertexWithVertex(v);
    }

    virtual void addVertexWithVertex(Vertex * const v) {
        vertices[v->getValue<void *>()] = v;
        adjacencies[v];
    }

    virtual void addEdgeFromVertices(Vertex *src, Vertex *tgt) =0;
    virtual void removeEdgeFromVertices(Vertex *src, Vertex *tgt) =0;

    std::map<void *, Vertex *> vertices;
    std::map<Vertex *, std::set<Edge>> adjacencies;
};

class DirectedPtrGraph : public PtrGraphBase {
public:
    DirectedPtrGraph() : indegrees() { }

    virtual ~DirectedPtrGraph() { }

    DirectedPtrGraph(const DirectedPtrGraph &that) :
        PtrGraphBase(that),
        indegrees() {
        for (Vertex *const v : that.allVertices()) {
            void *ptr = v->getValue<void*>();
            indegrees[getVertex(ptr)] = that.indegree(v);
        }
    }

    DirectedPtrGraph(DirectedPtrGraph &&that) :
        PtrGraphBase(std::move(that)), indegrees(std::move(that.indegrees)) { }


    int indegree(Vertex *v) const {
        return indegrees.at(v);
    }

protected:
    virtual void addVertexWithVertex(Vertex * const v) override {
        indegrees[v] = 0;
        PtrGraphBase::addVertexWithVertex(v);
    }

    virtual void addEdgeFromVertices(Vertex *src, Vertex *tgt) override {
        adjacencies[src].insert(Edge(src, tgt));
        indegrees[tgt]++;
    }

    virtual void removeEdgeFromVertices(Vertex *src, Vertex *tgt) override {
        adjacencies[src].erase(Edge(src, tgt));
        indegrees[tgt]--;
    }

private:
    std::map<Vertex *, int> indegrees;
};

class UndirectedPtrGraph : public PtrGraphBase {
public:
    UndirectedPtrGraph() { }
protected:
    virtual void addEdgeFromVertices(Vertex *src, Vertex *tgt) override {
        adjacencies[src].insert(Edge(src, tgt));
        adjacencies[tgt].insert(Edge(tgt, src));
    }

    virtual void removeEdgeFromVertices(Vertex *src, Vertex *tgt) override {
        adjacencies[src].erase(Edge(src, tgt));
        adjacencies[tgt].erase(Edge(tgt, src));
    }

};

#endif
