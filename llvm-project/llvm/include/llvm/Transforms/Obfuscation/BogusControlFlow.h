#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"

namespace llvm {

class BogusControlFlow : public PassInfoMixin<BogusControlFlow> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

  static bool isRequired() { return true; }
};

}
