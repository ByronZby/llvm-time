add_llvm_library( LLVMTime MODULE BUILDTREE_ONLY
  Time.cpp
  ProbeDecl.cpp
  Path.cpp

  PtrGraph.cpp
  Cycle.cpp
  PathProfiler.cpp
  SingleEntrySingleExitDAG.cpp
  TopologicalOrder.cpp
  
  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Time

  DEPENDS
  intrinsics_gen

  PLUGIN_TOOL
  opt
  )
