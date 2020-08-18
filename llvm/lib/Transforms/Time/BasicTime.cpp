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
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/IR/Attributes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

#define DEBUG_TYPE "time"

// STATISTIC(HelloCounter, "Counts number of functions greeted");

namespace {
  struct TimeAnalysis : public ModulePass {
    static char ID; // Pass identification, replacement for typeid

    int numAnalyzer;

    TimeAnalysis() : ModulePass(ID), numAnalyzer(0) {}

    StringRef getPassName() const override {
      return "TimeAnalysis";
    }

    bool runOnModule(Module &M) override {
      dbgs() << "Module: ";
      dbgs().write_escaped(M.getName()) << '\n';

      // Search for the timing functions to call

#define LOG(Name)                                            \
  LLVM_DEBUG(dbgs() << "Got " #Name ", type: ");             \
  LLVM_DEBUG(Name##Callee.getFunctionType()->print(dbgs())); \
  LLVM_DEBUG(dbgs() << '\n')

      // void *BasicTimeAnalyzer_create()
      FunctionCallee createCallee =
          M.getOrInsertFunction("BasicTimeAnalyzer_create",
                                 Type::getInt8PtrTy(M.getContext()));

      LOG(create);

      // void BasicTimeAnalyzer_destroy(void *);
      FunctionCallee destroyCallee =
          M.getOrInsertFunction("BasicTimeAnalyzer_destroy",
                                Type::getVoidTy(M.getContext()),
                                Type::getInt8PtrTy(M.getContext())
                                          ->getPointerTo());

      LOG(destroy);


      // void *BasicTimeAnalyzer_now()
      FunctionCallee nowCallee =
          M.getOrInsertFunction("BasicTimeAnalyzer_now",
                                Type::getInt8PtrTy(M.getContext()));

      LOG(now);

      // void BasicTimeAnalyzer_log_time_and_process(void *, void *, void *,
      //                                             int)
      FunctionCallee logProcessCallee =
          M.getOrInsertFunction("BasicTimeAnalyzer_log_time_and_process",
                                Type::getVoidTy(M.getContext()),
                                Type::getInt8PtrTy(M.getContext()),
                                Type::getInt8PtrTy(M.getContext()),
                                Type::getInt8PtrTy(M.getContext()),
                                Type::getInt32Ty(M.getContext()));

      LOG(logProcess);

      // Iterate through all functions and insert code for timing
      // instrumenetations
      for (Function &F : M) {
        if (F.hasFnAttribute(Attribute::TimeTarget)) {
          dbgs() << "Function to be timed: ";
          dbgs().write_escaped(F.getName()) << '\n';

          LLVM_DEBUG(dbgs() << "Creating global variable\n");

          GlobalVariable *obj =
            new GlobalVariable(M,
                               Type::getInt8PtrTy(M.getContext()),
                               false,
                               GlobalValue::LinkageTypes::CommonLinkage,
                               nullptr,
                               "analyzer_obj_" + std::to_string(numAnalyzer));

          obj->setInitializer(ConstantPointerNull::get(Type::getInt8PtrTy(M.getContext())));

          FunctionCallee initCallee = makeInitFunction(M, obj, createCallee);
          appendToGlobalCtors(M, cast<Function>(initCallee.getCallee()), 65535);

          FunctionCallee delCallee = makeDelFunction(M, obj, destroyCallee);
          appendToGlobalDtors(M, cast<Function>(delCallee.getCallee()), 65535);

          // insert start call at entry block
          BasicBlock &entry = F.getEntryBlock();
          Instruction *firstInst = &entry.front();
          /*
          auto loadObj = new LoadInst(Type::getInt8PtrTy(M.getContext()),
                                      obj,
                                      "loadobj",
                                      firstInst);
                                      */
          LLVM_DEBUG(dbgs() << "Inserting start time\n");
          auto *startTime = CallInst::Create(nowCallee, "startTime", firstInst);

          for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            if (isa<ReturnInst>(&*I)) {
              LLVM_DEBUG(dbgs() << "Inserting stop time and log\n");
              auto *stopTime = CallInst::Create(nowCallee,
                                                "stopTime",
                                                &*I);
              auto *loadObj = new LoadInst(Type::getInt8PtrTy(M.getContext()),
                                           obj,
                                           "loadObj",
                                           &*I);
              auto *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()),
                                            0);
              auto *log = CallInst::Create(logProcessCallee,
                                           {loadObj, startTime, stopTime, zero},
                                           "",
                                           &*I);
              (void) log;
            }
          }

          ++numAnalyzer;
        }
      }

      LLVM_DEBUG(dbgs() << "Done\n");
      return false;
    }

    FunctionCallee makeInitFunction(Module &M,
                                    GlobalVariable *GV,
                                    FunctionCallee cons) const {
      LLVM_DEBUG(dbgs() << "Making init function\n");

      FunctionCallee FC =
        M.getOrInsertFunction("__init_basic_time_analyzer"
                                + std::to_string(numAnalyzer),
                              Type::getVoidTy(M.getContext()));

      Function *initFunc = cast<Function>(FC.getCallee());

      BasicBlock *BB = BasicBlock::Create(M.getContext(), "entry", initFunc);

      auto *obj = CallInst::Create(cons, "obj", BB);
      auto *assign = new StoreInst(obj, GV, BB); (void) assign;
      auto *ret = ReturnInst::Create(M.getContext(), nullptr, BB); (void) ret;

      return FC;
    }

    FunctionCallee makeDelFunction(Module &M,
                                   GlobalVariable *GV,
                                   FunctionCallee del) const {
      LLVM_DEBUG(dbgs() << "Making del function\n");

      FunctionCallee FC =
        M.getOrInsertFunction("__del_basic_time_analyzer"
                                + std::to_string(numAnalyzer),
                              Type::getVoidTy(M.getContext()));
      Function *delFunc = cast<Function>(FC.getCallee());

      BasicBlock *BB = BasicBlock::Create(M.getContext(), "entry", delFunc);

      auto *obj = CallInst::Create(del, {GV}, "", BB); (void) obj;
      auto *ret = ReturnInst::Create(M.getContext(), nullptr, BB); (void) ret;

      return FC;
    }

    /*
    // We don't modify the program, so we preserve all analyses.
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
    }
    */

    static void registerTimeAnalysis(const PassManagerBuilder &,
                                     legacy::PassManagerBase &PM) {
      PM.add(new TimeAnalysis());
    }
  };
}

char TimeAnalysis::ID = 0;
static RegisterPass<TimeAnalysis>
X("time", "Inject code for time analysis instrumentation");
