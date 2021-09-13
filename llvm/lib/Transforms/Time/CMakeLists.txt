add_llvm_pass_plugin( LoopTime
  Time.cpp
  ProbeDecl.cpp
  Path.cpp

  GraphAlgorithms.cpp
  PtrGraph.cpp
  
  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Time

  DEPENDS
  intrinsics_gen
  )
