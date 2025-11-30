#ifndef LLVM_TRANSFORMS_OBFUSCATION_EXAMPLE_H
#define LLVM_TRANSFORMS_OBFUSCATION_EXAMPLE_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class ExampleObfuscation : public PassInfoMixin<ExampleObfuscation> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

  static bool isRequired() { return true; }
};

} // namespace llvm

#endif