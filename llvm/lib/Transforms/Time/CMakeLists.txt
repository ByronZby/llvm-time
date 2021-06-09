add_llvm_pass_plugin( LoopTime
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
  )
