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
    llvm::FunctionCallee header;
    llvm::Module         *_M;

    static Instrument *get(llvm::Module *M) {
        if (Instance._M == nullptr || Instance._M != M) {
            Instance.declare(M);
            Instance.placeCtorDtor(M);
            Instance._M = M;
        }

        return &Instance;
    }

private:
    // Singleton class
    Instrument() : _M(nullptr) { }

    Instrument(llvm::Module *M) : _M(M) { }

    void declare(llvm::Module *);
    void placeCtorDtor(llvm::Module *);

    static Instrument Instance;
};

} // namespace time

#endif
