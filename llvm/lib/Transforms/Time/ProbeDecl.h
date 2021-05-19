//===- ProbeDecl.h - Declarations for probing functions  --------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#ifndef PROBEDECL_H_INCLUDED_
#define PROBEDECL_H_INCLUDED_

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"

namespace LLVMTime {

struct Instrument {
    llvm::FunctionCallee initialize;
    llvm::FunctionCallee cleanup;
    llvm::FunctionCallee enter_loop;
    llvm::FunctionCallee exit_loop;
    llvm::FunctionCallee latch;
    llvm::FunctionCallee path;

    void declare(llvm::Module &);
    void placeCtorDtor(llvm::Module &);
};

} // namespace time

#endif
