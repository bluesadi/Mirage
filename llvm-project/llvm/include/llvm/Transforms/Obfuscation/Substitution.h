#ifndef LLVM_TRANSFORMS_OBFUSCATION_SUBSTITUTION_H
#define LLVM_TRANSFORMS_OBFUSCATION_SUBSTITUTION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Substitution : public PassInfoMixin<Substitution> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif
