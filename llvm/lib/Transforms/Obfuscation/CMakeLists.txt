add_llvm_component_library(LLVMObfuscation
  Example.cpp

  LINK_COMPONENTS
  Core
  Support
  TransformUtils
  )