#ifndef PARSER_H
#define PARSER_H

#include "program.h"
#include "types.h"
#include "utils/utils.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

namespace x64 {

    class InstructionParser {
        struct OpcodeBytes {
            std::vector<u8> bytes;

            size_t size() const { return bytes.size(); }

            u8 operator[](u8 i) const {
                assert(i < bytes.size());
                return bytes[i];
            }
        };
        static OpcodeBytes opcodeBytesFromString(std::string_view s);
    public:
        static std::unique_ptr<Program> parseFile(std::string filename);
        static std::unique_ptr<X86Instruction> parseInstructionLine(std::string_view s);
        static std::unique_ptr<X86Instruction> parseInstruction(const OpcodeBytes& opbytes, u32 address, std::string_view s);

        static std::vector<std::unique_ptr<Function>> parseSection(std::string_view filepath, std::string_view section);

    private:
        using line_iterator = std::vector<std::string>::const_iterator;
        static std::unique_ptr<Function> parseFunction(line_iterator& begin, line_iterator end);

        static std::unique_ptr<X86Instruction> parsePush(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parsePop(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseMov(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMovsx(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMovzx(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMovsxd(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseLea(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseAdd(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseAdc(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSub(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSbb(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseNeg(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseMul(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseImul(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseDiv(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseIdiv(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseAnd(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseOr(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseXor(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseNot(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseXchg(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseXadd(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseCall(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);

        static std::unique_ptr<X86Instruction> parseRet(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseLeave(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseHalt(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseNop(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseUd2(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCdq(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseInc(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseDec(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseShr(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseShl(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseShrd(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseShld(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseSar(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseRol(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        template<Cond cond>
        static std::unique_ptr<X86Instruction> parseSet(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseTest(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCmp(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCmpxchg(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseJmp(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJne(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJe(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJae(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJbe(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJge(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJle(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJa(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJb(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJg(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJl(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJs(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);
        static std::unique_ptr<X86Instruction> parseJns(const OpcodeBytes& opbytes, u32 address, std::string_view operands, std::string_view decorator);

        static std::unique_ptr<X86Instruction> parseBsr(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);
        static std::unique_ptr<X86Instruction> parseBsf(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString);

        static std::unique_ptr<X86Instruction> parseRepStringop(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseRepzStringop(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseRepnzStringop(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        template<Cond cond>
        static std::unique_ptr<X86Instruction> parseCmov(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseCwde(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseCdqe(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parsePxor(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovaps(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovabs(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovdqa(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovdqu(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovups(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseMovd(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovq(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseMovss(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseMovsd(const OpcodeBytes& opbytes, u32 address, std::string_view operands);

        static std::unique_ptr<X86Instruction> parseAddss(const OpcodeBytes& opbytes, u32 address, std::string_view operands);
        static std::unique_ptr<X86Instruction> parseAddsd(const OpcodeBytes& opbytes, u32 address, std::string_view operands);


    };

}

#endif