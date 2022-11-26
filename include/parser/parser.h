#ifndef PARSER_H
#define PARSER_H

#include "program.h"
#include "utils/utils.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x86 {

    class InstructionParser {
    public:
        static std::unique_ptr<Program> parseFile(std::string filename);
        static std::unique_ptr<Function> parseFunction(std::ifstream& file);
        static std::unique_ptr<X86Instruction> parseInstructionLine(std::string_view s);

    private:
        static std::unique_ptr<X86Instruction> parseInstruction(u32 address, std::string_view s);

        static std::unique_ptr<X86Instruction> parsePush(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parsePop(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseMov(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMovsx(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMovzx(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseLea(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseAdd(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseAdc(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSub(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSbb(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseNeg(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMul(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseImul(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseDiv(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseIdiv(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseAnd(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseOr(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseXor(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseNot(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseXchg(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseCall(u32 address, std::string_view operands, std::string_view decorator);

        static std::unique_ptr<X86Instruction> parseRet(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseLeave(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseHalt(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseNop(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseUd2(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCdq(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseInc(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseDec(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseShr(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseShl(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseShrd(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseShld(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSar(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseRol(u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseSeta(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetae(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetb(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetbe(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSete(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetg(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetge(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetl(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetle(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseSetne(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseTest(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCmp(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseJmp(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJne(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJe(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJae(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJbe(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJge(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJle(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJa(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJb(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJg(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJl(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJs(u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJns(u32 address, std::string_view operands, std::string_view decorator);

        static std::unique_ptr<X86Instruction> parseBsr(u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseBsf(u32 address, std::string_view operandsString);

    };

}

#endif