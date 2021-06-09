#ifndef LLVM_TRANSFORMS_TIME_TIME_H_
#define LLVM_TRANSFORMS_TIME_TIME_H_

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

namespace llvm {

/// Performs basic CFG simplifications to assist other loop passes.
class LoopTimePass : public PassInfoMixin<LoopTimePass> {
public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR, LPMUpdater &U);

  static bool isRequired() {
      return true;
  }

};
} // end namespace llvm

#endif
