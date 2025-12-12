#ifndef LLVM_TRANSFORMS_OBFUSCATION_FLATTENING_H
#define LLVM_TRANSFORMS_OBFUSCATION_FLATTENING_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Flattening : public PassInfoMixin<Flattening> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

  static bool isRequired() { return true; }
};

} // namespace llvm

#endif