//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

// #include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
using namespace llvm;

#define DEBUG_TYPE "time"

// STATISTIC(HelloCounter, "Counts number of functions greeted");

namespace {
  struct FuncAnalysis : public ModulePass {
    static char ID; // Pass identification, replacement for typeid

    int numAnalyzer;

    FuncAnalysis() : ModulePass(ID), numAnalyzer(0) {}

    StringRef getPassName() const override {
      return "FuncAnalysis";
    }

    bool runOnModule(Module &M) override {
      dbgs() << "Module: ";
      dbgs().write_escaped(M.getName()) << '\n';

      PointerType *voidptrty = Type::getInt8PtrTy(M.getContext());
      Type *voidty = Type::getVoidTy(M.getContext());

#define LOG(Name)                                            \
  LLVM_DEBUG(dbgs() << "Got " #Name ", type: ");             \
  LLVM_DEBUG(Name##Callee.getFunctionType()->print(dbgs())); \
  LLVM_DEBUG(dbgs() << '\n')

      // Search for the timing functions to call

      // void *FuncTimeAnalyzer_create()
      FunctionCallee createAnalyzerCallee =
        M.getOrInsertFunction("FuncTimeAnalyzer_create", voidptrty);

      LOG(createAnalyzer);

      // void *CallInfo_create()
      FunctionCallee createCallInfoCallee =
        M.getOrInsertFunction("CallInfo_create", voidptrty);

      LOG(createCallInfo);

      // void FuncTimeAnalyzer_destroy(void **);
      FunctionCallee destroyCallee =
        M.getOrInsertFunction("FuncTimeAnalyzer_destroy",
                              voidty,
                              voidptrty->getPointerTo());

      LOG(destroy);

      // void *FuncTimeAnalyzer_now()
      FunctionCallee nowCallee =
        M.getOrInsertFunction("FuncTimeAnalyzer_now",
                              voidptrty);

      LOG(now);

      // void FuncTimeAnalyzer_log_time_and_process(void *, void *, void *,
      //                                            void *)
      FunctionCallee logProcessCallee =
        M.getOrInsertFunction("FuncTimeAnalyzer_log_time_and_process",
                              voidty,
                              voidptrty,
                              voidptrty,
                              voidptrty,
                              voidptrty);

      LOG(logProcess);

      // Get call graph
      LLVM_DEBUG(dbgs() << "Creating call graph\n");
      CallGraph CG(M);

      CG.dump();

      return false;

      // Iterate through all functions and insert code for timing
      // instrumenetations
      for (Function &F : M) {
        if (F.hasFnAttribute(Attribute::TimeTarget)) {
          dbgs() << "Function to be timed: ";
          dbgs().write_escaped(F.getName()) << '\n';

          CallGraphNode *node = CG[&F];

          for (CallGraphNode::CallRecord &CR : *node) {
            Function *f = CR.second->getFunction();
            dbgs() << "This function calls " << f->getName() << '\n';
          }
        }
      }

      LLVM_DEBUG(dbgs() << "Done\n");
      return false;
    }

    /*
    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
    }
    */

  };
}

char FuncAnalysis::ID = 0;
static RegisterPass<FuncAnalysis>
X("func-time", "Inject code for func time analysis instrumentation");
