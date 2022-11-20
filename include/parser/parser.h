#ifndef PARSER_H
#define PARSER_H

#include "utils/utils.h"
#include "instructions.h"
#include "instructionhandler.h"
#include <memory>
#include <string_view>

namespace x86 {

    struct X86Instruction {
        explicit X86Instruction(u32 address) : address(address) { }
        virtual ~X86Instruction() = default;
        virtual void exec(InstructionHandler* handler) = 0;

        u32 address;
    };

    template<typename Instruction>
    struct InstructionWrapper : public X86Instruction {
        InstructionWrapper(u32 address, Instruction instruction) :
                X86Instruction(address), 
                instruction(std::move(instruction)) { }

        void exec(InstructionHandler* handler) override {
            return handler->exec(instruction);
        }

        Instruction instruction;
    };

    template<typename Instruction>
    inline std::unique_ptr<X86Instruction> make_wrapper(u32 address, Instruction instruction) {
        return std::make_unique<InstructionWrapper<Instruction>>(address, instruction);
    }

    class InstructionParser {
    public:
        static std::unique_ptr<X86Instruction> parseInstructionLine(std::string_view s);

    private:
        static std::unique_ptr<X86Instruction> parseInstruction(u32 address, std::string_view s);

        static std::unique_ptr<X86Instruction> parsePush(u32 address, std::string_view operands);

    };

}

#endif