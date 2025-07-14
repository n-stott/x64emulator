#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "x64/compiler/ir.h"
#include <memory>
#include <type_traits>
#include <vector>

namespace x64::ir {

    class OptimizationPass;
    struct LivenessAnalysis;

    class Optimizer {
    public:
        template<typename Pass>
        void addPass() {
            static_assert(std::is_base_of_v<OptimizationPass, Pass>);
            passes_.push_back(std::make_unique<Pass>());
        }

        struct Stats {
            u32 removedInstructions { 0 };
            u32 deadCode { 0 };
            u32 immediateReadback { 0 };
            u32 delayedReadback { 0 };
            u32 duplicateInstruction { 0 };
        };

        void optimize(IR& ir, Stats* stats = nullptr);

    private:
        std::vector<std::unique_ptr<OptimizationPass>> passes_;
    };
    
    class OptimizationPass {
    public:
        virtual ~OptimizationPass() = default;
        virtual bool optimize(IR*, Optimizer::Stats*) = 0;
    };
    
    class DeadCodeElimination : public OptimizationPass {
    public:
        DeadCodeElimination();
        ~DeadCodeElimination();
        bool optimize(IR*, Optimizer::Stats*) override;

    private:
        std::unique_ptr<LivenessAnalysis> analysis_;
        std::vector<size_t> removableInstructions_;
    };

    class ImmediateReadBackElimination : public OptimizationPass {
        bool optimize(IR*, Optimizer::Stats*) override;
    private:
        std::vector<size_t> removableInstructions_;
    };

    class DelayedReadBackElimination : public OptimizationPass {
        bool optimize(IR*, Optimizer::Stats*) override;
    private:
        std::vector<size_t> removableInstructions_;
    };

    class DuplicateInstructionElimination : public OptimizationPass {
        bool optimize(IR*, Optimizer::Stats*) override;
    private:
        std::vector<size_t> removableInstructions_;
    };
}

#endif