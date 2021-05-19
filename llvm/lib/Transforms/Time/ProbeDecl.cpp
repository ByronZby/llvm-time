//===- ProbeDecl.cpp - Declarations for probing functions  ------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//

#include "ProbeDecl.h"

#include "llvm/Transforms/Utils/ModuleUtils.h"

using namespace LLVMTime;
using namespace llvm;

#define MAKE_FUNC(name, ...) name = M.getOrInsertFunction("INSTRUMENT_" #name, __VA_ARGS__)

void Instrument::declare(Module &M) {
    auto *voidty = Type::getVoidTy(M.getContext());
    auto *i8ptrty = Type::getInt8PtrTy(M.getContext());

    MAKE_FUNC(initialize, voidty);
    MAKE_FUNC(cleanup, voidty);
    MAKE_FUNC(enter_loop, voidty, i8ptrty);
    MAKE_FUNC(exit_loop, voidty, i8ptrty);
    MAKE_FUNC(latch, voidty, i8ptrty);
    MAKE_FUNC(path, voidty, Type::getInt32Ty(M.getContext()));
}

void Instrument::placeCtorDtor(llvm::Module &M) {
    appendToGlobalCtors(M, cast<Function>(initialize.getCallee()), 65535);
    appendToGlobalDtors(M, cast<Function>(cleanup.getCallee()), 65535);
}


