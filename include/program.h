#ifndef PROGRAM_H
#define PROGRAM_H

#include "instructionhandler.h"
#include "instructionutils.h"
#include "utils/utils.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x64 {

    class InstructionHandler;

    struct X86Instruction {
        explicit X86Instruction(u64 address) : address(address) { }
        virtual ~X86Instruction() = default;
        virtual void exec(InstructionHandler* handler) const = 0;
        virtual std::string toString(const InstructionHandler* handler) const = 0;

        u64 address;
    };

    template<typename Instruction>
    struct InstructionWrapper : public X86Instruction {
        explicit InstructionWrapper(Instruction instruction) :
                X86Instruction(0xDEADC0DE),
                instruction(std::move(instruction)) { }

        InstructionWrapper(u64 address, Instruction instruction) :
                X86Instruction(address),
                instruction(std::move(instruction)) { }

        void exec(InstructionHandler* handler) const override {
            return handler->exec(instruction);
        }

        virtual std::string toString(const InstructionHandler* handler) const override {
            if constexpr(std::is_same_v<Instruction, CallDirect>) {
                if(handler) handler->resolveFunctionName(instruction);
            }
            return x64::utils::toString(instruction);
        }

        Instruction instruction;
    };

    struct ExecutableSection {
        u64 begin;
        u64 end;
        std::vector<std::unique_ptr<X86Instruction>> instructions;
        std::string filename;
    };

}

#endif