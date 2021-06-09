//===- Path.cpp -   --------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "Path.h"

#include "SingleEntrySingleExitDAG.h"
#include "PathProfiler.h"
#include "Cycle.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <list>
#include <stack>

using namespace llvm;

using BlockSet = SmallPtrSetImpl<BasicBlock *>;
using SESEDAG = SingleEntrySingleExitDAG;

static void constructGraph(const BlockSet &BBs, DirectedPtrGraph &G);
static void reduceCycles(const Loop *L, SESEDAG &G);
static void getProperLoop(const Loop *L, SmallPtrSetImpl<BasicBlock *> &Result);

static void printProfile(SESEDAG &G, const PathProfiler &PP, raw_ostream &OS);

Value *LLVMTime::instrumentPathProfile(llvm::Loop *L, raw_ostream &OS) {
    SESEDAG G;
    SmallPtrSet<BasicBlock *, 16> BBs;

    getProperLoop(L, BBs);
    constructGraph(BBs, G);
    reduceCycles(L, G);

    G.update();

    PathProfiler PP(G);

    printProfile(G, PP, OS);

    Function *F = L->getHeader()->getParent();
    Instruction *InsertPt = &*L->getLoopPreheader()->getFirstInsertionPt();

    Type *PathNumType = Type::getInt32Ty(F->getContext());
    Value *PathNumPtr = new AllocaInst(PathNumType, 0, "pathnumptr", InsertPt);

    InsertPt = &*L->getHeader()->getFirstInsertionPt();
    new StoreInst(ConstantInt::get(PathNumType, 0), PathNumPtr, InsertPt);

    for (const auto &Iter : PP) {
        const Edge &E = Iter.first;
        const int Inc = Iter.second;

        BasicBlock *From = E.src->getValue<BasicBlock*>();
        BasicBlock *To = E.dest->getValue<BasicBlock*>();

        dbgs() << "Splitting Edge between " << static_cast<void*>(From) << " : " << From->getName() << " and "
               << static_cast<void*>(To) << " : " << To->getName() << "\n";

        BasicBlock *NewBlock = SplitEdge(E.src->getValue<BasicBlock *>(), E.dest->getValue<BasicBlock *>());
        L->addBlockEntry(NewBlock);

        InsertPt = &*NewBlock->getFirstInsertionPt();
        Value *PathNum = new LoadInst(PathNumType, PathNumPtr, "pathnum", InsertPt);
        Value *NewPathNum;
        if (Inc > 0) {
            NewPathNum = BinaryOperator::Create(Instruction::Add, PathNum, ConstantInt::get(PathNumType, Inc), "newpathnum", InsertPt);
        } else {
            NewPathNum = BinaryOperator::Create(Instruction::Sub, PathNum, ConstantInt::get(PathNumType, -Inc), "newpathnum", InsertPt);
        }

        new StoreInst(NewPathNum, PathNumPtr, InsertPt);
    }

    return PathNumPtr;
}

static void constructGraph(const BlockSet &BBs, DirectedPtrGraph &G) {
    for (BasicBlock *BB : BBs) {
        G.addVertex(BB);
    }

    for (BasicBlock *BB : BBs) {
        for (succ_iterator S = succ_begin(BB), E = succ_end(BB); S != E; ++S) {
            if (BBs.contains(*S)) {
                dbgs() << static_cast<void*>(BB) << " : " << BB->getName()
                       << " -> " << static_cast<void*>(*S) << " : " << (*S)->getName() << "\n";
                G.addEdge(BB, *S);
            }
        }
    }
}

static void reduceCycles(const Loop *L, SESEDAG &G) {
    G.removeEdge(L->getLoopLatch(), L->getHeader());
    
    assert(!Cycle(G).hasCycle());
}

static void getProperLoop(const Loop *L, SmallPtrSetImpl<BasicBlock *> &Result) {
    DominatorTree DT(*L->getHeader()->getParent());
    Result.clear();

    for (auto *BB : L->getBlocks()) {
        dbgs() << "Collect: " << BB->getName() << "\n";
        if (isPotentiallyReachable(L->getLoopLatch(), BB, nullptr, &DT)) {
            Result.insert(BB);
        }
    }
}

template <class Function>
static void DFS(SESEDAG &G, Vertex *Src, Vertex *Dest, std::list<Vertex *> &Path, Function F) {
    //TODO: make this non-recursive
    if (Src == Dest) {
        F(Path);
    } else {
        for (const Edge &E : G.adj(Src)) {
            Vertex *W = E.dest;

            Path.push_back(W);
            DFS(G, W, Dest, Path, F);
            Path.pop_back();
        }
    }
}


template <class Function>
static void findAllPaths(SESEDAG &G, Function F) {
    std::list<Vertex *> Path;
    Vertex *Entry = G.getEntryVertex(), *Exit = G.getExitVertex();
    Path.push_back(Entry);

    DFS(G, Entry, Exit, Path, F);
}

static void printBasicBlock(const BasicBlock *BB, raw_ostream &OS) {
    StringSet<MallocAllocator> Set;
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
        Set.insert(S);
    }

    OS << '"' << BB->getName() << '"' << " : [\n";

    bool needsComma = false;
    for (const auto &Iter : Set) {
        if (needsComma) OS << ",\n";
        needsComma = true;
        OS << "    " << '"' << Iter.getKey() << '"' ;
    }
    OS << "]";

}

static void printProfile(SESEDAG &G, const PathProfiler &PP, raw_ostream &OS) {
    OS << "{\n";
    OS << "\"BasicBlocks\": {\n";
    bool needsComma = false;
    for (auto *V : G.allVertices()) {
        const BasicBlock *BB = V->getValue<BasicBlock *>();
        if (needsComma) OS << ",\n";
        needsComma = true;
        printBasicBlock(BB, OS);
    }
    OS << "},\n";
    OS << "\"Paths\": {\n";

    needsComma = false;
    // Assign identifier to basicblock
    findAllPaths(G, [&PP, &OS, &needsComma] (const std::list<Vertex *> &Path) {
            int PathNum = 0;
            auto I = Path.begin();
            const auto E = Path.end();

            for (Vertex *V = *I, *W = *(++I); I != E; V = W, W = *(++I)) {
                PathNum += PP.getEdgeVal(V, W);
            }

            if (needsComma) OS << ",\n";
            needsComma = true;
            OS << "    " << '"' << PathNum << '"' << ": [\n";

            bool needsInnerComma = false;
            for (Vertex *V : Path) {
                BasicBlock *BB = V->getValue<BasicBlock *>();
                if (needsInnerComma) OS << ",\n";
                needsInnerComma = true;
                OS << "        " << '"' << BB->getName() << '"';
            }

            OS << "    ]";
    });

    OS << "\n}\n}\n";
}

