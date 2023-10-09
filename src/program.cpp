#include "program.h"
#include "interpreter/verify.h"
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {

    Function::Function(u64 address, std::string name, std::vector<const X86Instruction*> instructions) :
        address(address), name(std::move(name)), instructions(std::move(instructions)) {
        demangledName = boost::core::demangle(this->name.c_str());
    }

    void Function::print() const {
        fmt::print("{}\n", name);
        for(const auto& ins : instructions) {
            fmt::print(stderr, "{:x} {}\n", ins->address, ins->toString(nullptr));
        }
    }


}