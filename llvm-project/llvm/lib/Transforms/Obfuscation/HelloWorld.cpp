#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/Example.h"

namespace llvm {

static cl::opt<bool> enableHello("enable-hello-obfu", cl::init(false), cl::desc("Enable My Hello World Pass"));

PreservedAnalyses ExampleObfuscation::run(Function &F, FunctionAnalysisManager &AM) {
    if (!enableHello)
        return PreservedAnalyses::all();

    errs() << "Hello World! I am visiting function: " << F.getName() << "\n";

    return PreservedAnalyses::all();
}

} // namespace llvm
