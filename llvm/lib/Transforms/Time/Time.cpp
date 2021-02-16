//===- Time.cpp - Loop Timing Instrumentation -------------------*- C++ -*-===//
// This pass inserts instrumentation to every loops in the program
//===----------------------------------------------------------------------===//

#include "ProbeDecl.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopInfo.h"

#include "llvm/ADT/SmallVector.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/DebugLoc.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "time"

namespace {

class TimePass : public LoopPass {
public:
    static char ID;
    TimePass() : LoopPass(ID) { }

    bool runOnLoop(Loop *L, LPPassManager &LPM) override;
    bool doInitialization(Loop *L, LPPassManager &LPM) override;

private:
    LLVMTime::Instrument instrument;

    void placeEntryInstrument(BasicBlock *header, GlobalVariable *name);
    void placeExitInstrument(BasicBlock *exit, GlobalVariable *name);
    void placeLatchInstrument(BasicBlock *latch, GlobalVariable *name);
};

} // anonymous namespace

static std::string getDebugLocString(const Loop *L);
static GlobalVariable *declareStringLiteral(const std::string &s, Module *M);
static Constant *referStringLiteral(GlobalVariable *strlit, Module *M);

bool TimePass::runOnLoop(Loop *L, LPPassManager &LPM) {
    dbgs() << "Enter\n";
    std::string name = getDebugLocString(L);
    dbgs() << "Loop: " << name << "\n";

    if (!L->isLoopSimplifyForm()) {
        WithColor::warning() << name << " is not in simplified form; skipped\n";
        return false;
    }

    Module *M = L->getHeader()->getModule();
    GlobalVariable *loopName = declareStringLiteral(name, M);

    BasicBlock *preheader = L->getLoopPreheader();
    assert(preheader && "Preheader");
    placeEntryInstrument(preheader, loopName);

    BasicBlock *latch = L->getLoopLatch();
    assert(latch && "Latch");
    placeLatchInstrument(latch, loopName);

    SmallVector<BasicBlock *, 8> exits;
    L->getExitBlocks(exits);

    for (BasicBlock *exit : exits) {
        placeExitInstrument(exit, loopName);
    }

    return true;
}

bool TimePass::doInitialization(Loop *L, LPPassManager &LPM) {
    static bool initialized = false;
    if (initialized) {
        return false;
    }

    Module *M = L->getHeader()->getModule();
    instrument.declare(*M);
    instrument.placeCtorDtor(*M);

    initialized = true;
    return true;
}

void TimePass::placeEntryInstrument(BasicBlock *header, GlobalVariable *name) {
    Module *M = header->getModule();
    auto insertPt = header->getTerminator();

    dbgs() << "Placing an entry point at ";
    insertPt->getDebugLoc().print(dbgs());
    dbgs() << "\n";

    CallInst::Create(instrument.enter_loop,
                     {referStringLiteral(name, M)},
                     "",
                     &*insertPt);
}

void TimePass::placeExitInstrument(BasicBlock *exit, GlobalVariable *name) {
    Module *M = exit->getModule();
    auto insertPt = exit->getFirstInsertionPt();

    dbgs() << "Placing an exit point at ";
    insertPt->getDebugLoc().print(dbgs());
    dbgs() << "\n";

    CallInst::Create(instrument.exit_loop,
                     {referStringLiteral(name, M)},
                     "",
                     &*insertPt);
}

void TimePass::placeLatchInstrument(BasicBlock *latch, GlobalVariable *name) {
    Module *M = latch->getModule();
    auto insertPt = latch->getTerminator();

    dbgs() << "Placing an exit point at ";
    insertPt->getDebugLoc().print(dbgs());
    dbgs() << "\n";

    CallInst::Create(instrument.latch,
                     {referStringLiteral(name, M)},
                     "",
                     &*insertPt);
}

static std::string getDebugLocString(const Loop *L) {
    std::string result;
    static int loop_id = 0;
    if (L) {
        raw_string_ostream os(result);
        if (const DebugLoc loopDebugLoc = L->getStartLoc())
           loopDebugLoc.print(os);
        else {
           // Just print the module name
           os << L->getHeader()->getParent()->getParent()->getModuleIdentifier();
           os << ": loop " << loop_id++;
        }
        os.flush();
    }
    return result;
}

GlobalVariable *declareStringLiteral(const std::string &s, Module *M) {
    Constant *val = ConstantDataArray::getString(M->getContext(), s);
    auto *gv = new GlobalVariable(
            *M,
            val->getType(),
            true, /* isConstant */
            GlobalValue::PrivateLinkage,
            val,
            ".loopidentifier"/* name */
            );
    gv->setAlignment(MaybeAlign(1));

    return gv;
}

Constant *referStringLiteral(GlobalVariable *strlit, Module *M) {
    std::vector<Constant *> indices;
    ConstantInt *const_zero = ConstantInt::get(
            Type::getInt32Ty(M->getContext()),
            0, /* value */
            true
            );
    indices.push_back(const_zero);
    indices.push_back(const_zero);

    return ConstantExpr::getGetElementPtr(strlit->getType()->getElementType(), strlit, indices, true /*isInBound*/);
}


char TimePass::ID = 0;
static RegisterPass<TimePass> X("time", "Timing Instrumentation", false, false);
