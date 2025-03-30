#include "x64/compiler/optimizer.h"
#include "x64/tostring.h"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace x64::ir {

    void Optimizer::optimize(IR& ir) {
        // fmt::print("Before: {}\n", ir.instructions.size());
        // for(const auto& ins : ir.instructions) {
        //     fmt::print("  {}\n", ins.toString());
        // }
        while(true) {
            bool didSomething = false;
            for(const auto& pass : passes_) {
                didSomething |= pass->optimize(&ir);
            }
            if(!didSomething) break;
        }
        // fmt::print("After: {}\n", ir.instructions.size());
        // for(const auto& ins : ir.instructions) {
        //     fmt::print("  {}\n", ins.toString());
        // }
    }

}