//===- GraphGenerator.h - Generate random graphs -------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef GRAPHGENERATOR_H_INCLUDED_
#define GRAPHGENERATOR_H_INCLUDED_

class UndirectedPtrGraph;
class DirectedPtrGraph;

namespace GraphGenerator {

void simple(UndirectedPtrGraph &g, int V, int E);
void simple(DirectedPtrGraph &g, int V, int E);

void path(UndirectedPtrGraph &g, int V);
void path(DirectedPtrGraph &g, int V);

void cycle(UndirectedPtrGraph &g, int V);
void cycle(DirectedPtrGraph &g, int V);

void DAG(DirectedPtrGraph &g, int V, int E);

void naturalLoop(DirectedPtrGraph &g, int V, int E);

} // namespace GraphGenerator

        


#endif

