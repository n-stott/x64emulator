#include "program.h"
#include "interpreter/verify.h"
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {

    Function::Function(u64 address, std::string name, std::vector<const X86Instruction*> instructions) :
        address(address), name(std::move(name)), instructions(std::move(instructions)) {
        demangledName = boost::core::demangle(this->name.c_str());
    }

    const Function* Program::findUniqueFunction(std::string_view name) const {
#ifndef NDEBUG
        const Function* ret = nullptr;
        for(const Function& func : functions) {
            if(func.name == name) {
                verify(ret == nullptr, [&]() {
                    fmt::print(stderr, "Function {} is not unique\n", name);
                });
                ret = &func;
            }
        }
        return ret;
#else
        for(const Function& func : functions) {
            if(func.name == name) return &func;
        }
        return nullptr;
#endif
    }

    const Function* Program::findFunction(u64 address, std::string_view name) const {
        for(const Function& func : functions) {
            if(func.address == address && func.name == name) return &func;
        }
        return nullptr;
    }

    const Function* Program::findFunctionByAddress(u64 address) const {
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