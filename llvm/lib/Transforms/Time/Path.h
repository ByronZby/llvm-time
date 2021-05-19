//===- Path.h -   --------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef PATH_H_INCLUDED_
#define PATH_H_INCLUDED_

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

namespace LLVMTime {

llvm::Value *instrumentPathProfile(llvm::Loop *L, llvm::raw_ostream &OS);

}


#endif
