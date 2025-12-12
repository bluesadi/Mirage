add_llvm_component_library(LLVMObfuscation
  HelloWorld.cpp
  Flattening.cpp
  Substitution.cpp
  Utils.cpp

  LINK_COMPONENTS
  Core
  Support
  TransformUtils
  )