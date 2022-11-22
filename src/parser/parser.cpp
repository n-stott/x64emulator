#include "parser/parser.h"
#include <cassert>
#include <charconv>
#include <cstdlib>
#include <optional>
#include <vector>
#include <fmt/core.h>

namespace x86 {

    std::vector<std::string_view> split(std::string_view sv, char separator) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        size_t next = 0;
        while(next != sv.size()) {
            next = sv.find(separator, pos);
            if(next == std::string::npos) next = sv.size();
            result.push_back(std::string_view(sv.begin()+pos, next-pos));
            pos = next+1;
        }
        return result;
    }

    std::vector<std::string_view> split(std::string_view sv, std::string_view separators) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        size_t next = 0;
        while(next != std::string::npos && next != sv.size()) {
            next = std::string::npos;
            for(char separator : separators) next = std::min(next, sv.find(separator, pos));
            if(next == std::string::npos) next = sv.size();
            result.push_back(std::string_view(sv.begin()+pos, next-pos));
            pos = next+1;
        }
        return result;
    }

    std::string_view strip(std::string_view sv) {
        const char* skippedCharacters = " \n\t";
        size_t first = sv.find_first_not_of(skippedCharacters);
        size_t last = sv.find_last_not_of(skippedCharacters);
        if(first != std::string::npos && last != std::string::npos) {
            return sv.substr(first, last+1-first);
        } else {
            return "";
        }
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInstructionLine(std::string_view s) {
        std::vector<std::string_view> parts = split(s, '\t');
        assert(parts.size() == 3);
        // for(auto sv : parts) {
        //     auto stripped = strip(sv);
        //     fmt::print("_{}_  ", stripped);
        // }
        // fmt::print("\n");

        std::string_view addressString = parts[0];
        assert(addressString.back() == ':');

        u32 address = 0;
        auto result = std::from_chars(addressString.data(), addressString.data()+addressString.size(), address, 16);
        assert(result.ec == std::errc{});
        

        std::string_view instructionString = parts[2];
        auto ptr = parseInstruction(address, instructionString);
        fmt::print("{:40} {:40}: {}\n", instructionString, (!!ptr ? ptr->toString() : "???"), (!!ptr ? "ok" : "fail"));
        return ptr;
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInstruction(u32 address, std::string_view s) {
        size_t nameEnd = s.find_first_of(' ');
        std::string_view name = strip(nameEnd < s.size() ? s.substr(0, nameEnd) : s);
        std::string_view rest = strip(nameEnd < s.size() ? s.substr(nameEnd) : "");
        std::vector<std::string_view> operandsAndDecorator = split(rest, "<>");
        std::string_view operands = strip(operandsAndDecorator.size() > 0 ? operandsAndDecorator[0] : "");
        std::string_view decorator = strip(operandsAndDecorator.size() > 1 ? operandsAndDecorator[1] : "");
        // fmt::print("{} _{}_ _{}_ _{}_\n", operandsAndDecorator.size(), name, operands, decorator);
        if(name == "push") return parsePush(address, operands);
        if(name == "mov") return parseMov(address, operands);
        if(name == "add") return parseAdd(address, operands);
        if(name == "sub") return parseSub(address, operands);
        if(name == "call") return parseCall(address, operands, decorator);
        return {};
    }

    std::optional<R32> asRegister(std::string_view sv) {
        if(sv == "ebp") return R32::EBP;
        if(sv == "esp") return R32::ESP;
        if(sv == "eax") return R32::EAX;
        if(sv == "ebx") return R32::EBX;
        if(sv == "ecx") return R32::ECX;
        if(sv == "edx") return R32::EDX;
        return {};
    }

    std::optional<u8> asImmediate8(std::string_view sv) {
        if(sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 2) return {};
        u8 immediate = 0;
        auto result = std::from_chars(sv.data(), sv.data()+2, immediate, 16);
        if(result.ptr != sv.data()+2) return {};
        assert(result.ec == std::errc{});
        return immediate;
    }

    std::optional<u32> asImmediate32(std::string_view sv) {
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 8) return {};
        u32 immediate = 0;
        auto result = std::from_chars(sv.data(), sv.data()+sv.size(), immediate, 16);
        if(result.ptr != sv.data()+sv.size()) return {};
        assert(result.ec == std::errc{});
        return immediate;
    }

    std::optional<i32> asDisplacement(std::string_view sv) {
        if(sv.size() == 0) return {};
        i32 sign = +1;
        if(sv[0] == '+') {
            // ok
            sv = sv.substr(1);
        } else if(sv[0] == '-') {
            sign = -1;
            sv = sv.substr(1);
        } else {
            return {};
        }
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 8) return {};
        i32 immediate = 0;
        auto result = std::from_chars(sv.data(), sv.data()+sv.size(), immediate, 16);
        if(result.ptr != sv.data()+sv.size()) return {};
        assert(result.ec == std::errc{});
        return sign*immediate;
    }

    bool isEncoding(std::string_view sv) {
        return sv.size() >= 2 && sv.front() == '[' && sv.back() == ']';
    }

    std::optional<BD> asBaseAndDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        auto base = asRegister(sv.substr(0, 3));
        auto displacement = asDisplacement(sv.substr(3));
        if(base && displacement) return BD{base.value(), displacement.value()};
        return {};
    }

    std::optional<Addr<Size::DWORD, BD>> asDoubleBD(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "DWORD") return {};
        if(parts[1] != "PTR") return {};
        auto bd = asBaseAndDisplacement(parts[2]);
        if(!bd) return {};
        return Addr<Size::DWORD, BD>{bd.value()};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parsePush(u32 address, std::string_view operandString) {
        auto r32 = asRegister(operandString);
        if(r32) return make_wrapper<Push<R32>>(address, r32.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMov(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister(operands[0]);
        auto r32src = asRegister(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r32dst && r32src) return make_wrapper<Mov<R32, R32>>(address, Mov<R32, R32>{r32dst.value(), r32src.value()});
        if(addrDoubleBDdst && r32src) return make_wrapper<Mov<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBDdst && imm32src) return make_wrapper<Mov<Addr<Size::DWORD, BD>, u32>>(address, addrDoubleBDdst.value(), imm32src.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAdd(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister(operands[0]);
        auto r32src = asRegister(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r32dst && imm32src) return make_wrapper<Add<R32, u32>>(address, r32dst.value(), imm32src.value());
        if(r32dst && r32src) return make_wrapper<Add<R32, R32>>(address, r32dst.value(), r32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSub(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister(operands[0]);
        auto imm8src = asImmediate8(operands[1]);
        if(r32dst && imm8src) return make_wrapper<Sub<R32, SignExtended<u8>>>(address, r32dst.value(), SignExtended<u8>{imm8src.value()});
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCall(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<CallDirect>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

}