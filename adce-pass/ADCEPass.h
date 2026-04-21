#ifndef LLVM_TRANSFORMS_UTILS_ASSIGNMENT3_H
#define LLVM_TRANSFORMS_UTILS_ASSIGNMENT3_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"

#include <queue>
#include <set>

namespace llvm {

    class Assignment3Pass : public PassInfoMixin<Assignment3Pass> {

        public:

            PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

        private:

            Function*                   func;               // The function that we are working on
            std::queue<Instruction*>    worklist;           // Instructions that just became live and need to be processed
            std::set<BasicBlock*>       reachableBlocks;    // The set of reachable basic blocks
            std::set<Instruction*>      liveInstrictions;   // The set of live instructions

            void markLive(Instruction *I);
            bool isTriviallyLive(Instruction &I) const;

    };

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_ASSIGNMENT3_H