#ifndef PARSER_H
#define PARSER_H

#include "utils/utils.h"
#include "instructions.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include <memory>
#include <string_view>

namespace x86 {

    struct X86Instruction {
        explicit X86Instruction(u32 address) : address(address) { }
        virtual ~X86Instruction() = default;
        virtual void exec(InstructionHandler* handler) = 0;
        virtual std::string toString() const = 0;

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

        virtual std::string toString() const override {
            return x86::utils::toString(instruction);
        }

        Instruction instruction;
    };

    template<typename Instruction, typename... Args>
    inline std::unique_ptr<X86Instruction> make_wrapper(u32 address, Args... args) {
        return std::make_unique<InstructionWrapper<Instruction>>(address, Instruction{args...});
    }

    class InstructionParser {
    public:
        static void parseFile(std::string filename);
        static std::unique_ptr<X86Instruction> parseInstructionLine(std::string_view s);

    private:
        static std::unique_ptr<X86Instruction> parseInstruction(u32 address, std::string_view s);

        static std::unique_ptr<X86Instruction> parsePush(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parsePop(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseMov(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseLea(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseAdd(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSub(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseAnd(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseOr(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseXor(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseXchg(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseCall(u32 address, std::string_view operands, std::string_view decorator);

        static std::unique_ptr<X86Instruction> parseRet(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseLeave(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseHalt(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseNop(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseUd2(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseShr(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSar(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseTest(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCmp(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseJmp(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJne(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJe(u32 address, std::string_view operands, std::string_view decorator);

    };

}

#endif