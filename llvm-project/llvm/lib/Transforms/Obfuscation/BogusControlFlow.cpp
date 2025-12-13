#include "llvm/Transforms/Obfuscation/BogusControlFlow.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include <vector>

using namespace llvm;

static cl::opt<bool> enableBogusControlFlow("enable-bcf-obfu", cl::init(false),
                                            cl::desc("Enable Bogus Control Flow Obfuscation Pass"));

namespace llvm {

static BasicBlock *cloneBasicBlock(BasicBlock *BB) {
    ValueToValueMapTy VMap;
    BasicBlock *cloneBB = CloneBasicBlock(BB, VMap, "cloneBB", BB->getParent());

    BasicBlock::iterator origI = BB->begin();
    for (Instruction &I : *cloneBB) {
        for (unsigned idx = 0; idx < I.getNumOperands(); ++idx) {
            if (Value *mapped = MapValue(I.getOperand(idx), VMap))
                I.setOperand(idx, mapped);
        }

        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        I.getAllMetadata(MDs);
        for (auto &pair : MDs) {
            if (MDNode *mappedMD = MapMetadata(pair.second, VMap))
                I.setMetadata(pair.first, mappedMD);
        }

        I.setDebugLoc(origI->getDebugLoc());
        ++origI;
    }

    return cloneBB;
}

static Value *createBogusCmp(BasicBlock *insertBlock) {
    Module *M = insertBlock->getModule();
    LLVMContext &Ctx = M->getContext();

    auto *xptr = new GlobalVariable(*M, Type::getInt32Ty(Ctx), false, GlobalValue::CommonLinkage,
                                    ConstantInt::get(Type::getInt32Ty(Ctx), 0), "x");
    auto *yptr = new GlobalVariable(*M, Type::getInt32Ty(Ctx), false, GlobalValue::CommonLinkage,
                                    ConstantInt::get(Type::getInt32Ty(Ctx), 0), "y");

    IRBuilder<> builder(Ctx);
    builder.SetInsertPoint(insertBlock);

    LoadInst *x = builder.CreateLoad(Type::getInt32Ty(Ctx), xptr);
    LoadInst *y = builder.CreateLoad(Type::getInt32Ty(Ctx), yptr);

    Value *cond1 = builder.CreateICmpSLT(y, ConstantInt::get(Type::getInt32Ty(Ctx), 10));
    Value *op1 = builder.CreateAdd(x, ConstantInt::get(Type::getInt32Ty(Ctx), 1));
    Value *op2 = builder.CreateMul(op1, x);
    Value *op3 = builder.CreateURem(op2, ConstantInt::get(Type::getInt32Ty(Ctx), 2));
    Value *cond2 = builder.CreateICmpEQ(op3, ConstantInt::get(Type::getInt32Ty(Ctx), 0));

    return builder.CreateOr(cond1, cond2, "bcf.cond");
}

PreservedAnalyses BogusControlFlow::run(Function &F, FunctionAnalysisManager &AM) {
    if (!enableBogusControlFlow)
        return PreservedAnalyses::all();

    std::vector<BasicBlock *> origBB;
    for (BasicBlock &BB : F)
        origBB.push_back(&BB);

    for (BasicBlock *BB : origBB) {
        if (isa<InvokeInst>(BB->getTerminator()) || BB->isEHPad())
            continue;

        BasicBlock *headBB = BB;
        BasicBlock *bodyBB = BB->splitBasicBlock(BB->getFirstNonPHIOrDbgOrLifetime(), "bodyBB");
        BasicBlock *tailBB = bodyBB->splitBasicBlock(bodyBB->getTerminator(), "endBB");

        BasicBlock *cloneBB = cloneBasicBlock(bodyBB);

        headBB->getTerminator()->eraseFromParent();
        bodyBB->getTerminator()->eraseFromParent();
        cloneBB->getTerminator()->eraseFromParent();

        Value *cond1 = createBogusCmp(headBB);
        Value *cond2 = createBogusCmp(bodyBB);

        BranchInst::Create(bodyBB, cloneBB, cond1, headBB);
        BranchInst::Create(tailBB, cloneBB, cond2, bodyBB);
        BranchInst::Create(bodyBB, cloneBB);
    }

    return PreservedAnalyses::all();
}

} // namespace llvm
