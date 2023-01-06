#ifndef PROGRAM_H
#define PROGRAM_H

#include "utils/utils.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x86 {

    class InstructionHandler;

    struct X86Instruction {
        explicit X86Instruction(u32 address) : address(address) { }
        virtual ~X86Instruction() = default;
        virtual void exec(InstructionHandler* handler) const = 0;
        virtual std::string toString() const = 0;

        u32 address;
    };

    struct Function {
        u32 address;
        std::string name;
        std::vector<std::unique_ptr<X86Instruction>> instructions;

        void print() const;
    };

    struct Program {
        std::string filepath;
        std::string filename;
        std::vector<Function> functions;

        Program() = default;
        virtual ~Program() = default;
        Program(Program&&) = default;

        virtual const Function* findUniqueFunction(std::string_view name) const;
        virtual const Function* findFunction(u32 address, std::string_view name) const;
        virtual const Function* findFunctionByAddress(u32 address) const;
    };

}

#endif