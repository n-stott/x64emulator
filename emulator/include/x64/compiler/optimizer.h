#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "x64/compiler/ir.h"
#include <memory>
#include <type_traits>
#include <vector>

namespace x64::ir {

    class OptimizationPass {
    public:
        virtual bool optimize(IR*) = 0;
    };

    class Optimizer {
    public:
        template<typename Pass>
        void addPass() {
            static_assert(std::is_base_of_v<OptimizationPass, Pass>);
            passes_.push_back(std::make_unique<Pass>());
        }

        void optimize(IR& ir);

    private:
        std::vector<std::unique_ptr<OptimizationPass>> passes_;
    };
    
    class DeadCodeElimination : public OptimizationPass {
        bool optimize(IR*) override;
    };

    class ImmediateReadBackElimination : public OptimizationPass {
        bool optimize(IR*) override;
    };

    class DelayedReadBackElimination : public OptimizationPass {
        bool optimize(IR*) override;
    };

    class DuplicateInstructionElimination : public OptimizationPass {
        bool optimize(IR*) override;
    };
}

#endif