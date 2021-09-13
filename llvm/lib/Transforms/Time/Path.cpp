//===- Path.cpp -   --------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "Path.h"
#include "PtrGraph.h"
#include "GraphAlgorithms.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringSet.h"

#include "llvm/Analysis/CFG.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <list>
#include <stack>
#include <tuple>

using namespace llvm;

static SmallPtrSet<BasicBlock *, 16> findReachableBlocks(const Loop *L);

static DirectedPtrGraph<BasicBlock *>
constructGraph(const SmallPtrSetImpl<BasicBlock *> &BBs);

static void serializeProfile(const DirectedPtrGraph<BasicBlock *> &G,
                             const PathProfiler<BasicBlock *> &PP,
                             raw_ostream &OS);

static Value* insertInstrumentation(Loop *L,
                                    PathProfiler<BasicBlock *> &PP);

static void addBlockEntryToParents(Loop *L, BasicBlock *BB);

Value* LLVMTime::instrumentPathProfile(Loop *L, raw_ostream &OS) {
    DirectedPtrGraph<BasicBlock *> G = constructGraph(findReachableBlocks(L));
    G.disconnect(L->getLoopLatch(), L->getHeader());

    PathProfiler<BasicBlock *> PP(G);

    serializeProfile(G, PP, OS);

    return insertInstrumentation(L, PP);
}

static SmallPtrSet<BasicBlock *, 16> findReachableBlocks(const Loop *L) {
    SmallPtrSet<BasicBlock *, 16> BBs;
    DominatorTree DT(*L->getHeader()->getParent());

    BasicBlock *Latch = L->getLoopLatch();

    for (auto *BB : L->getBlocks()) {
        dbgs() << "Collect: " << BB->getName() << "\n";
        if (isPotentiallyReachable(Latch, BB, nullptr, &DT)) {
            BBs.insert(BB);
        }
    }

    return BBs;
}


static DirectedPtrGraph<BasicBlock *>
constructGraph(const SmallPtrSetImpl<BasicBlock *> &BBs) {
    DirectedPtrGraph<BasicBlock *> G;

    for (BasicBlock *BB : BBs) {
        G.insert(BB);
    }

    for (BasicBlock *Src : BBs) {
        for (succ_iterator S = succ_begin(Src), E = succ_end(Src);
             S != E; ++S) {
            BasicBlock *Dest = *S;
            if (BBs.contains(Dest)) {
                dbgs() << static_cast<void*>(Src) << " : " << Src->getName()
                       << " -> " << static_cast<void*>(Dest) << " : "
                       << Dest->getName() << "\n";

                G.connect(Src, Dest);
            }
        }
    }

    return G;
}

template <class Function>
static void DFS(const DirectedPtrGraph<BasicBlock *> &G,
                BasicBlock *Src,
                BasicBlock *Dest,
                std::list<BasicBlock *> &Path,
                Function F) {
    //TODO: make this non-recursive

    if (Src == Dest) {
        F(Path);
    } else {
        for (BasicBlock *Adjacent : G.getAdj(Src)) {
            Path.push_back(Adjacent);
            DFS(G, Adjacent, Dest, Path, F);
            Path.pop_back();
        }
    }
}

template <class Function>
static void findAllPaths(const DirectedPtrGraph<BasicBlock *> &G,
                         Function F) {
    std::list<BasicBlock *> Path;
    BasicBlock *Entry = G.getEntry(), *Exit = G.getExit();

    Path.push_back(Entry);

    DFS(G, Entry, Exit, Path, F);
}

static void printBasicBlock(const BasicBlock *BB, raw_ostream &OS) {
    StringSet<MallocAllocator> LineNumbers;
    for (const auto &I : *BB) {
        std::string S;
        raw_string_ostream Stream(S);
        if (const DebugLoc &Loc = I.getDebugLoc()) {
            auto *Scope = cast<DIScope>(Loc.getScope());
            Stream << Scope->getFilename() << ':' << Loc.getLine();
        } else {
            Stream << "unavailable";
        }
        Stream.flush();
        LineNumbers.insert(S);
    }

    OS << '"' << BB->getName() << '"' << " : [\n";

    bool needsComma = false;
    for (const auto &Iter : LineNumbers) {
        if (needsComma) OS << ",\n";
        needsComma = true;
        OS << "    " << '"' << Iter.getKey() << '"' ;
    }
    OS << "]";
}

static void serializeProfile(const DirectedPtrGraph<BasicBlock *> &G,
                             const PathProfiler<BasicBlock *> &PP,
                             raw_ostream &OS) {
    OS << "{\n";
    OS << "\"BasicBlocks\": {\n";
    bool needsComma = false;
    for (BasicBlock *BB : G.getAllNodes()) {
        if (needsComma) OS << ",\n";
        needsComma = true;
        printBasicBlock(BB, OS);
    }
    OS << "},\n";
    OS << "\"Paths\": {\n";

    needsComma = false;
    // Assign identifier to basicblock
    findAllPaths(G,
                 [&PP, &OS, &needsComma] (const std::list<BasicBlock *> &Path) {
            int PathNum = 0;
            auto PathBegin = Path.begin();
            const auto PathEnd = Path.end();

            for (BasicBlock *Src = *PathBegin, *Dest = *(++PathBegin);
                 PathBegin != PathEnd;
                 Src = Dest, Dest = *(++PathBegin)) {
                PathNum += PP.getEdgeVal(Src, Dest);
            }

            if (needsComma) OS << ",\n";
            needsComma = true;
            OS << "    " << '"' << PathNum << '"' << ": [\n";

            bool needsInnerComma = false;
            for (BasicBlock *BB : Path) {
                if (needsInnerComma) OS << ",\n";
                needsInnerComma = true;
                dbgs() << "Printing " << static_cast<void*>(BB) << " : "
                       << (BB->hasName() ? BB->getName() : "No Name")
                       << "\n";
                OS << "        "
                   << '"' << (BB->hasName() ? BB->getName() : "No Name") << '"';
            }

            OS << "    ]";
    });

    OS << "\n}\n}\n";
}

static Value* insertInstrumentation(Loop *L,
                                    PathProfiler<BasicBlock *> &PP) {
    IRBuilder<> Builder(&*L->getLoopPreheader()->getFirstInsertionPt());

    Type  *PathNumType = Builder.getInt32Ty();
    Value *PathNumPtr  = Builder.CreateAlloca(PathNumType, 0, nullptr,
                                              "pathnumptr");

    Builder.SetInsertPoint(&*L->getHeader()->getFirstInsertionPt());
    Builder.CreateStore(Builder.getInt32(0), PathNumPtr);

    for (const auto &Pair : PP) {
        BasicBlock *Src, *Dest;
        int Inc;

        std::pair<BasicBlock *, BasicBlock *> Edge;
        std::tie(Edge, Inc) = Pair;
        std::tie(Src, Dest) = Edge;

        dbgs() << "Splitting Edge between " << static_cast<void*>(Src) << " : "
               << Src->getName() << " and " << static_cast<void*>(Dest)
               << " : " << Dest->getName() << "\n";

        BasicBlock *NewBlock = SplitEdge(Src, Dest);
        dbgs() << "New block : " << static_cast<void*>(NewBlock) << "\n";
        addBlockEntryToParents(L, NewBlock);

        Builder.SetInsertPoint(&*NewBlock->getFirstInsertionPt());

        Value *PathNum = Builder.CreateLoad(PathNumType, PathNumPtr, "pathnum");

        Value *NewPathNum;
        if (Inc > 0) {
            NewPathNum = Builder.CreateAdd(PathNum,               /* LHS */
                                           Builder.getInt32(Inc), /* RHS */
                                           "newpathnum");
        } else {
            NewPathNum = Builder.CreateSub(PathNum,
                                           Builder.getInt32(-Inc),
                                           "newpathnum");
        }
        Builder.CreateStore(NewPathNum, PathNumPtr);
    }

    return PathNumPtr;
}

static void addBlockEntryToParents(Loop *L, BasicBlock *BB) {
    do {
        L->addBlockEntry(BB);
    } while ((L = L->getParentLoop()) != nullptr);
}


