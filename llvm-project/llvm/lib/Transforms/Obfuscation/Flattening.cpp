#include "llvm/Transforms/Obfuscation/Flattening.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/ErrorHandling.h"
#include <vector>
#include <map>
#include <random>
#include <set>

using namespace llvm;

static cl::opt<bool> EnableFlattening(
    "enable-fla-obfu",
    cl::init(false),
    cl::desc("Enable Control Flow Flattening Obfuscation Pass")
);

namespace llvm {

struct FlatteningImpl {
    Function &F;
    FunctionAnalysisManager &AM;
    
    std::map<BasicBlock*, uint32_t> blockIDs;
    AllocaInst *swVar;

    FlatteningImpl(Function &F, FunctionAnalysisManager &AM) : F(F), AM(AM) {}

    bool run();

private:
    void lowerSwitches();
    void demotePHIs();
    void demoteLiveOuts();
    void flatten();
    void updateTerminator(BasicBlock *BB, BasicBlock *target);
};

bool FlatteningImpl::run() {
    if (F.size() <= 1) return false;

    lowerSwitches();
    demotePHIs();
    demoteLiveOuts();
    flatten();

    return true;
}

void FlatteningImpl::lowerSwitches() {
    LowerSwitchPass().run(F, AM);
}

void FlatteningImpl::demotePHIs() {
    std::vector<PHINode*> phis;
    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
            if (PHINode *PN = dyn_cast<PHINode>(&I)) {
                phis.push_back(PN);
            }
        }
    }
    for (PHINode *PN : phis) {
        DemotePHIToStack(PN, F.getEntryBlock().getTerminator());
    }
}

void FlatteningImpl::demoteLiveOuts() {
    std::vector<Instruction*> toDemote;
    for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
            if (isa<AllocaInst>(&I) || I.isTerminator()) continue;
            
            bool liveOut = false;
            for (User *U : I.users()) {
                if (Instruction *UI = dyn_cast<Instruction>(U)) {
                    if (UI->getParent() != &BB) {
                        liveOut = true;
                        break;
                    }
                }
            }
            
            if (liveOut) {
                toDemote.push_back(&I);
            }
        }
    }
    
    for (Instruction *I : toDemote) {
        DemoteRegToStack(*I, false, F.getEntryBlock().getTerminator());
    }
}

void FlatteningImpl::flatten() {
    std::vector<BasicBlock*> origBB;
    // Collect blocks
    for (BasicBlock &BB : F) {
        if (!BB.isEHPad() && &BB != &F.getEntryBlock()) {
            origBB.push_back(&BB);
        }
    }
    
    if (origBB.empty()) return;

    BasicBlock *entryBB = &F.getEntryBlock();
    
    BasicBlock *loopEntry = BasicBlock::Create(F.getContext(), "loopEntry", &F, entryBB);
    BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "loopEnd", &F, entryBB);
    loopEntry->moveAfter(entryBB);
    loopEnd->moveAfter(loopEntry);

    IRBuilder<> builder(entryBB->getTerminator());
    swVar = builder.CreateAlloca(builder.getInt32Ty(), nullptr, "swVar");

    // Assign Random IDs
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<uint32_t> dist;
    std::set<uint32_t> usedIDs;

    for (BasicBlock *BB : origBB) {
        uint32_t id = dist(g);
        while (usedIDs.count(id) || id == 0) { // Ensure unique and non-zero
            id = dist(g);
        }
        blockIDs[BB] = id;
        usedIDs.insert(id);
    }

    // Update Entry Block
    updateTerminator(entryBB, loopEntry);

    // Setup Loop Entry
    builder.SetInsertPoint(loopEntry);
    LoadInst *loadSw = builder.CreateLoad(builder.getInt32Ty(), swVar, "swVal");
    SwitchInst *switchInst = builder.CreateSwitch(loadSw, loopEnd, origBB.size());

    // Setup Loop End
    builder.SetInsertPoint(loopEnd);
    builder.CreateBr(loopEntry);

    // Process Original Blocks
    for (BasicBlock *BB : origBB) {
        BB->moveAfter(loopEnd);
        switchInst->addCase(builder.getInt32(blockIDs[BB]), BB);
        updateTerminator(BB, loopEnd);
    }
    
    // Process EH Pads (update their terminators if they jump to flattened blocks)
    for (BasicBlock &BB : F) {
        if (BB.isEHPad()) {
            updateTerminator(&BB, loopEntry);
        }
    }
}

void FlatteningImpl::updateTerminator(BasicBlock *BB, BasicBlock *target) {
    Instruction *term = BB->getTerminator();
    IRBuilder<> builder(term);

    if (BranchInst *br = dyn_cast<BranchInst>(term)) {
        if (br->isUnconditional()) {
            BasicBlock *succ = br->getSuccessor(0);
            if (blockIDs.count(succ)) {
                builder.CreateStore(builder.getInt32(blockIDs[succ]), swVar);
                builder.CreateBr(target);
                br->eraseFromParent();
            }
        } else {
            BasicBlock *succT = br->getSuccessor(0);
            BasicBlock *succF = br->getSuccessor(1);
            
            bool T_in = blockIDs.count(succT);
            bool F_in = blockIDs.count(succF);
            
            if (T_in && F_in) {
                Value *cond = br->getCondition();
                Value *valT = builder.getInt32(blockIDs[succT]);
                Value *valF = builder.getInt32(blockIDs[succF]);
                Value *select = builder.CreateSelect(cond, valT, valF);
                builder.CreateStore(select, swVar);
                builder.CreateBr(target);
                br->eraseFromParent();
            } else if (T_in && !F_in) {
                BasicBlock *newT = BasicBlock::Create(F.getContext(), "flat_T", &F);
                newT->moveAfter(BB);
                IRBuilder<> bT(newT);
                bT.CreateStore(bT.getInt32(blockIDs[succT]), swVar);
                bT.CreateBr(target);
                
                br->setSuccessor(0, newT);
            } else if (!T_in && F_in) {
                BasicBlock *newF = BasicBlock::Create(F.getContext(), "flat_F", &F);
                newF->moveAfter(BB);
                IRBuilder<> bF(newF);
                bF.CreateStore(bF.getInt32(blockIDs[succF]), swVar);
                bF.CreateBr(target);
                
                br->setSuccessor(1, newF);
            }
        }
    } else if (InvokeInst *invoke = dyn_cast<InvokeInst>(term)) {
        BasicBlock *normalDest = invoke->getNormalDest();
        
        if (blockIDs.count(normalDest)) {
            BasicBlock *trampoline = BasicBlock::Create(F.getContext(), "trampoline", &F);
            trampoline->moveAfter(BB);
            invoke->setNormalDest(trampoline);
            
            IRBuilder<> bTrampoline(trampoline);

            if (!invoke->getType()->isVoidTy()) {
                IRBuilder<> bEntry(F.getEntryBlock().getFirstNonPHI());
                AllocaInst *invokeAlloca = bEntry.CreateAlloca(invoke->getType(), nullptr, "invoke_spill");
                
                bTrampoline.CreateStore(invoke, invokeAlloca);
                
                std::vector<User*> users;
                for (User *U : invoke->users()) {
                    Instruction *userInst = dyn_cast<Instruction>(U);
                    if (userInst && userInst->getParent() != trampoline) {
                        users.push_back(U);
                    }
                }
                
                for (User *U : users) {
                    Instruction *userInst = cast<Instruction>(U);
                    IRBuilder<> bUser(userInst);
                    LoadInst *val = bUser.CreateLoad(invoke->getType(), invokeAlloca, "invoke_reload");
                    U->replaceUsesOfWith(invoke, val);
                }
            }
            
            bTrampoline.CreateStore(bTrampoline.getInt32(blockIDs[normalDest]), swVar);
            bTrampoline.CreateBr(target);
        }
    }
}

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &AM) {
    if (!EnableFlattening)
        return PreservedAnalyses::all();

    FlatteningImpl impl(F, AM);
    if (impl.run())
        return PreservedAnalyses::none();
    
    return PreservedAnalyses::all();
}

}