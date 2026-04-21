#include "llvm/Transforms/Utils/Assignment3.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

void Assignment3Pass::markLive(Instruction *I) {
    if (I == nullptr) return;

    if (liveInstrictions.find(I) == liveInstrictions.end()) {
        liveInstrictions.insert(I);
        worklist.push(I);
    }
}

bool Assignment3Pass::isTriviallyLive(Instruction &I) const {
    return I.mayWriteToMemory() || I.mayHaveSideEffects() || I.isTerminator();
}

PreservedAnalyses Assignment3Pass::run(Function &F, FunctionAnalysisManager &AM) {

    // Initialize structures
    func = &F;
    liveInstrictions.clear();
    reachableBlocks.clear();
    worklist = std::queue<Instruction*>();
    bool changed = false;

    // Mark trivially live and trivially dead instructions
    for (BasicBlock *BB : depth_first(&F.getEntryBlock())) {
        reachableBlocks.insert(BB);

        for (auto It = BB->begin(); It != BB->end(); ) {
            Instruction *I = &*It;
            ++It; // advance before possible deletion

            if (isTriviallyLive(*I)) {
                markLive(I);
            } else if (I->use_empty()) {
                I->dropAllReferences();
                I->eraseFromParent();
                changed = true;
            }
        }
    }

    // Process worklist to find new live instructions
    while (!worklist.empty()) {
        Instruction *I = worklist.front();
        worklist.pop();

        BasicBlock *BB = I->getParent();
        if (reachableBlocks.find(BB) == reachableBlocks.end()) {
            continue;
        }

        for (Use &Op : I->operands()) {
            if (Instruction *OpInst = dyn_cast<Instruction>(Op.get())) {
                markLive(OpInst);
            }
        }
    }

    // Delete all instructions that are not live
    for (BasicBlock &BB : F) {
        if (reachableBlocks.find(&BB) == reachableBlocks.end()) {
            continue;
        }

        for (auto It = BB.begin(); It != BB.end(); ) {
            Instruction *I = &*It;
            ++It; // advance before possible deletion

            if (liveInstrictions.find(I) == liveInstrictions.end()) {
                I->dropAllReferences();
                I->eraseFromParent();
                changed = true;
            }
        }
    }

    if (changed) {
        return PreservedAnalyses::none();
    } else {
        return PreservedAnalyses::all();
    }
}