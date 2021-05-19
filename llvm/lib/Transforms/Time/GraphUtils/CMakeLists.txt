add_llvm_library( GraphUtils MODULE
  PtrGraph.cpp
  Cycle.cpp
  GraphGenerator.cpp
  PathProfiler.cpp
  SingleEntrySingleExitDAG.cpp
  TopologicalOrder.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Time/GraphUtils
  )

