#include "llvm/Transforms/Obfuscation/Substitution.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Obfuscation/Utils.h"

using namespace llvm;

static cl::opt<bool> enableSubstitution("enable-sub-obfu", cl::init(false),
                                        cl::desc("Enable Instruction Substitution Obfuscation Pass"));

static cl::opt<double> substitutionRate("sub-obfu-rate", cl::init(0.5), cl::Hidden,
                                        cl::desc("Probability [0..1] to substitute a supported instruction"));

namespace llvm {

static bool isSupportedType(Type *Ty) { return Ty->isIntegerTy(); }

static bool shouldApply() { return Rng::getInstance().next01() < substitutionRate; }

static Instruction *substituteAnd(IRBuilder<> &builder, Value *x, Value *y) {
    // x & y => ~((~x) | (~y))
    Value *nx = builder.CreateNot(x);
    Value *ny = builder.CreateNot(y);
    Value *orVal = builder.CreateOr(nx, ny);
    Value *res = builder.CreateNot(orVal);
    return cast<Instruction>(res);
}

static Instruction *substituteOr(IRBuilder<> &builder, Value *x, Value *y) {
    // x | y => ~((~x) & (~y))
    Value *nx = builder.CreateNot(x);
    Value *ny = builder.CreateNot(y);
    Value *andVal = builder.CreateAnd(nx, ny);
    Value *res = builder.CreateNot(andVal);
    return cast<Instruction>(res);
}

static Instruction *substituteXor(IRBuilder<> &builder, Value *x, Value *y) {
    // x ^ y => (x | y) & ~(x & y)
    Value *orVal = builder.CreateOr(x, y);
    Value *andVal = builder.CreateAnd(x, y);
    Value *nandVal = builder.CreateNot(andVal);
    Value *res = builder.CreateAnd(orVal, nandVal);
    return cast<Instruction>(res);
}

static Instruction *substituteMul(IRBuilder<> &builder, Value *x, Value *y) {
    // x * k => sum of shifted adds for constant k
    if (auto *constInt = dyn_cast<ConstantInt>(y)) {
        APInt k = constInt->getValue();
        if (k.isZero())
            return cast<Instruction>(ConstantInt::get(x->getType(), 0));

        Value *acc = ConstantInt::get(x->getType(), 0);
        unsigned bitWidth = k.getBitWidth();
        for (unsigned i = 0; i < bitWidth; ++i) {
            if (k[i]) {
                Value *term = (i == 0) ? x : builder.CreateShl(x, ConstantInt::get(x->getType(), i));
                acc = builder.CreateAdd(acc, term);
            }
        }
        return cast<Instruction>(acc);
    }
    return nullptr;
}

static Instruction *substituteAdd(IRBuilder<> &builder, Value *x, Value *y) {
    // x + y => (x ^ y) + ((x & y) << 1)
    Value *xorVal = builder.CreateXor(x, y);
    Value *andVal = builder.CreateAnd(x, y);
    Value *shl1 = builder.CreateShl(andVal, ConstantInt::get(andVal->getType(), 1));
    Value *res = builder.CreateAdd(xorVal, shl1);
    return cast<Instruction>(res);
}

static Instruction *substituteSub(IRBuilder<> &builder, Value *x, Value *y) {
    // x - y => x + (~y) + 1 (two's complement)
    Value *notY = builder.CreateNot(y);
    Value *plus = builder.CreateAdd(x, notY);
    Value *res = builder.CreateAdd(plus, ConstantInt::get(plus->getType(), 1));
    return cast<Instruction>(res);
}

static bool trySubstitute(Instruction &inst) {
    if (!isSupportedType(inst.getType()))
        return false;

    IRBuilder<> builder(&inst);

    if (auto *binOp = dyn_cast<BinaryOperator>(&inst)) {
        Value *x = binOp->getOperand(0);
        Value *y = binOp->getOperand(1);
        if (!isSupportedType(x->getType()) || !isSupportedType(y->getType()))
            return false;

        if (!shouldApply())
            return false;

        Instruction *newInst = nullptr;
        switch (binOp->getOpcode()) {
        case Instruction::And:
            newInst = substituteAnd(builder, x, y);
            break;
        case Instruction::Or:
            newInst = substituteOr(builder, x, y);
            break;
        case Instruction::Xor:
            newInst = substituteXor(builder, x, y);
            break;
        case Instruction::Mul:
            newInst = substituteMul(builder, x, y);
            break;
        case Instruction::Add:
            newInst = substituteAdd(builder, x, y);
            break;
        case Instruction::Sub:
            newInst = substituteSub(builder, x, y);
            break;
        default:
            break;
        }

        if (newInst) {
            newInst->setName(inst.getName());
            inst.replaceAllUsesWith(newInst);
            inst.eraseFromParent();
            return true;
        }
    }

    return false;
}

PreservedAnalyses Substitution::run(Function &F, FunctionAnalysisManager &AM) {
    if (!enableSubstitution)
        return PreservedAnalyses::all();

    bool changed = false;
    // Collect instructions first to avoid iterator invalidation
    SmallVector<Instruction *, 64> worklist;
    for (BasicBlock &bb : F)
        for (Instruction &inst : bb)
            worklist.push_back(&inst);

    for (Instruction *ip : worklist)
        if (ip->getParent())
            changed |= trySubstitute(*ip);

    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace llvm
