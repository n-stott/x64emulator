#ifndef PARSER_H
#define PARSER_H

#include "program.h"
#include "instructions.h"
#include "utils/utils.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x86 {

    class InstructionParser {
    public:
        static std::unique_ptr<Program> parseFile(std::string filename);
        static std::unique_ptr<X86Instruction> parseInstructionLine(std::string_view s);
        static std::unique_ptr<X86Instruction> parseInstruction(u32 address, std::string_view s);

    private:
        using line_iterator = std::vector<std::string>::const_iterator;
        static std::unique_ptr<Function> parseFunction(line_iterator& begin, line_iterator end);

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

        template<Cond cond>
        static std::unique_ptr<X86Instruction> parseSet(u32 address, std::string_view operands);

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

        static std::unique_ptr<X86Instruction> parseRepStringop(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseRepzStringop(u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseRepnzStringop(u32 address, std::string_view operands);

        template<Cond cond>
        static std::unique_ptr<X86Instruction> parseCmov(u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseCwde(u32 address, std::string_view operands);

    };

}

#endif