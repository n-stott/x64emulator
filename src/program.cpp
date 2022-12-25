#include "program.h"
#include <fmt/core.h>

namespace x86 {

    const Function* Program::findFunction(std::string_view name) const {
        for(const Function& func : functions) {
            if(func.name == name) return &func;
        }
        return nullptr;
    }

    const Function* Program::findFunctionByAddress(u32 address) const {
        for(const Function& func : functions) {
            if(func.address == address) return &func;
        }
        return nullptr;
    }

    void Function::print() const {
        fmt::print("{}\n", name);
        for(const auto& ins : instructions) {
            fmt::print(stderr, "{:x} {}\n", ins->address, ins->toString());
        }
    }


}