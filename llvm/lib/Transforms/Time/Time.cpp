//===- Time.cpp - Loop Timing Instrumentation -------------------*- C++ -*-===//
// This pass inserts instrumentation to every loops in the program
//===----------------------------------------------------------------------===//

#include "ProbeDecl.h"
#include "Path.h"

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"

#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/Pass.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace LLVMTime;

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

    SmallPtrSet<const BasicBlock *, 8> visited;
    for (BasicBlock *exit : exits) {
        if (!visited.contains(exit)) {
            visited.insert(exit);
            placeExitInstrument(exit, loopName);
        }
    }

    L->print(dbgs(), 0, true);

    dbgs() << "Done... Trying Path Profiling\n";

    if (!L->isInnermost()) {
        dbgs() << "Loop is not the innermost...Skip\n";
        return true;
    }

    std::error_code EC;
    raw_fd_ostream fs("PathProfile.json", EC);
    assert(!EC && "Can't open pathprofile file\n");

    Value *PathNumPtr = instrumentPathProfile(L, fs);

    BasicBlock *Latch = L->getLoopLatch();
    Instruction *InsertPt = &*Latch->getFirstInsertionPt();
    IRBuilder<> Builder(InsertPt);

    Value *PathNum = Builder.CreateLoad(Type::getInt32Ty(Builder.getContext()), PathNumPtr, "pathnum");
    Builder.CreateCall(instrument.path, {PathNum});


    dbgs() << "Done... Returning \n\n";
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
    auto debugLoc = insertPt->getDebugLoc();

    dbgs() << "Placing an entry point at ";
    debugLoc.print(dbgs());
    dbgs() << "\n";

    auto CI = CallInst::Create(instrument.enter_loop,
                               {referStringLiteral(name, M)},
                               "",
                               &*insertPt);
    CI->setDebugLoc(debugLoc);
}

void TimePass::placeExitInstrument(BasicBlock *exit, GlobalVariable *name) {
    Module *M = exit->getModule();
    auto insertPt = exit->getFirstInsertionPt();
    auto debugLoc = insertPt->getDebugLoc();

    dbgs() << "Placing an exit point at ";
    debugLoc.print(dbgs());
    dbgs() << "\n";

    auto CI = CallInst::Create(instrument.exit_loop,
                               {referStringLiteral(name, M)},
                               "",
                               &*insertPt);
    CI->setDebugLoc(debugLoc);
}

void TimePass::placeLatchInstrument(BasicBlock *latch, GlobalVariable *name) {
    Module *M = latch->getModule();
    auto insertPt = latch->getTerminator();
    auto debugLoc = insertPt->getDebugLoc();

    dbgs() << "Placing an exit point at ";
    debugLoc.print(dbgs());
    dbgs() << "\n";

    auto CI = CallInst::Create(instrument.latch,
                               {referStringLiteral(name, M)},
                               "",
                               &*insertPt);
    CI->setDebugLoc(debugLoc);
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

static GlobalVariable *declareStringLiteral(const std::string &s, Module *M) {
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

static Constant *referStringLiteral(GlobalVariable *strlit, Module *M) {
    std::vector<Constant *> indices;
    ConstantInt *const_zero = ConstantInt::get(
            Type::getInt32Ty(M->getContext()),
            0, /* value */
            true
            );
    indices.push_back(const_zero);
    indices.push_back(const_zero);

    return ConstantExpr::getGetElementPtr(strlit->getType()->getElementType(),
                                          strlit,
                                          indices,
                                          true /*isInBound*/);
}

char TimePass::ID = 0;
static RegisterPass<TimePass> X("time", "Timing Instrumentation", false, false);
